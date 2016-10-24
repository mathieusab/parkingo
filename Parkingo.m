clc
clear all
close all

t_max = 500; % temps max
t = 0:1:60*t_max; % echelle de temps en heures
E_batt_t = zeros(length(t),1);
ensol = zeros(length(t),1);

T_park_car = 5 * 60; %secondes

Sunrise = 9;
Sunset = 17;
T_soleil = Sunset - Sunrise; %heures par jour
Midday = (Sunrise+(Sunset-Sunrise)/2);

T_HCSR04_on = 238e-3; %secondes
DeepSleep = 1; %secondes
Time_on = 350e-3; %sec
Cycle_complet = DeepSleep + Time_on;

Rapport_on = Time_on / Cycle_complet;
Rapport_off = DeepSleep / Cycle_complet;
%% BMP180

I_BMP_on = 5e-6; % Amperes
I_BMP_off = 3e-6; % Amperes
U_BMP = 3; %Volt

P_BMP_on = U_BMP * I_BMP_on; % Puissance en Watt
P_BMP_off = U_BMP * I_BMP_off; % Puissance en Watt

P_BMP_moy = P_BMP_on * Rapport_on + P_BMP_off * Rapport_off;

%% HCSR04

I_HCSR04_on = 15e-3;
I_HCSR04_off = 2e-3;
U_HCSR04 = 5;

P_HCSR04_on = U_HCSR04 * I_HCSR04_on;
P_HCSR04_off = U_HCSR04 * I_HCSR04_off;

P_HCSR04_moy = P_HCSR04_on * Rapport_on + P_HCSR04_off * Rapport_off;

%% TD1208R

I_TD1208R_on = 32e-3;
I_TD1208R_off = 1.8e-6;
U_TD1208R = 3;

P_TD1208R_on = U_TD1208R * I_TD1208R_on;
P_TD1208R_off = U_TD1208R * I_TD1208R_off;

P_TD1208R_moy = P_TD1208R_on * Rapport_on + P_TD1208R_off * Rapport_off;

%% STM32

U_STM32 = 3.6;
I_STM32_on = 105e-3;
I_STM32_off = 0.8e-6;

P_STM32_on = U_STM32 * I_STM32_on;
P_STM32_off = U_STM32 * I_STM32_off;

P_STM32_moy = P_STM32_on * Rapport_on + P_STM32_off * Rapport_off;

%% Batterie

Q_batt = 1050e-3; %A.h
U_batt = 3.7; %V
E_batt = 3.9; % Watt-heure

%% Chargeur

U_chargeur = 5;
I_charge = 500e-3;
I_decharge = 1;
P_charge = U_chargeur * I_charge;
P_decharge = U_chargeur * I_decharge;

%% Cellule solaire

U_sol = 5.5;
I_sol = 540e-3;
P_sol_max = U_sol * I_sol; %puissance dans les conditions 

Irradiance_max = 1000; %W/m^2
Irradiance_h = 350; %W/m^2

P_sol_max_h = P_sol_max * Irradiance_h / Irradiance_max; % en hiver

%% Courbe solaire

x = [Sunrise*60 Midday*60 Sunset*60];
y = [0 1 0];
p = polyfit(x,y,2);
a = p(1);
b = p(2);
c = p(3);

%% Duree sur batt

P_tot_decharge = P_STM32_moy + P_HCSR04_moy;
P_tot_charge = P_sol_max_h;

duree_decharge = E_batt / P_tot_decharge;
duree_charge = E_batt / P_tot_charge;
courbe_sol(t_max*60+1) = 0;
for cpt = t
    if cpt ~= 0 
        if mod(cpt,24*60) > Sunrise*60 % apres 9h
            if mod(cpt,24*60) < Sunset*60 % avant 17h
                ensol(cpt+1) = 1;
                courbe_sol(cpt+1) = (a * mod((cpt+1),1440).^2 + b * mod((cpt+1),1440) + c) * ensol(cpt+1);
            else
                courbe_sol(cpt+1) = 0;
            end
        end
        if ensol(cpt) == 1 % si il y a du soleil
            E_batt_t(cpt+1) = E_batt_t(cpt) + P_tot_charge*courbe_sol(cpt+1)/60 - P_tot_decharge/60;
            if E_batt_t(cpt+1) > E_batt
                E_batt_t(cpt+1) = E_batt_t(cpt);
            end
        end
        if ensol(cpt) == 0 % si il n'y a pas du soleil
            E_batt_t(cpt+1) = E_batt_t(cpt) - P_tot_decharge/60;
            if E_batt_t(cpt+1) < 0
                E_batt_t(cpt+1) = 0;
            end
        end
    end
end

figure
XMIN = 0;
XMAX = 60*t_max;
YMIN = 0;
YMAX = 4;
subplot(2,1,1)
plot(t,courbe_sol)
ylabel('Courbe du soleil')
axis([XMIN XMAX YMIN-0.1 1.1])
subplot(2,1,2)
plot(t,E_batt_t)
ylabel('Energie dans la batterie')
xlabel('temps')
axis([XMIN XMAX YMIN-0.1 YMAX])
