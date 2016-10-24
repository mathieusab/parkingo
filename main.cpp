#include <stdio.h>
#include "mbed.h"
#include "HCSR04.h"
#include "AutomationElements.h"
#include "BMP180.h"
#include "WakeUp.h"

Serial pc(USBTX, USBRX); // serie USB

HCSR04 sensor(PC_14, PC_13); // capteur ultrason

I2C i2c(I2C_SDA, I2C_SCL);
BMP180 bmp180(&i2c);

bool carPresent = false; // voiture presente ? de base non
unsigned int car = 0;

float * value; // pointeur sur le tableau de 10 valeurs
float average = 0; // moyenne des 10 valeurs
float threshold = 600; //seuil en mm (60 cm)
float sampleTime = 3; // 1 capture toutes les 60 secondes (pour tester : 1capt/3sec)
float temp; // temperature
int pressure; //pression

int batt = 60; // valeur pour batterie A MODIFIER

int i   = 0; // compteur de boucle
int cpt = 0; // compteur de boucle

typedef enum
{
    Start,              //départ
    DeepSleep,          //veille
    SendCarTempPres,    //envoi des données
    MesMeteoBatt,       //mesure de la température, pression et niveau de batterie
    MesUS               //mesure de la présence de la voiture
}
State;

//fonction de mesure de 10 valeurs de la distance
float * getMesure()
{
    static float func_value[10];
    static int cpt = 0; //compteur de boucle
    
    for(cpt = 0; cpt < 10; cpt++)
    {
        func_value[cpt] = sensor.getDistance_mm();
    }
    return func_value;
}

//fonction de calcul de la moyenne
float getAverage()
{
    static float sum = 0; //somme des 10 valeurs
    static int cpt = 0; //compteur de boucle
    float func_average = 0; //moyenne
    
    sum = 0;
    
    for(cpt = 0; cpt < 10; cpt++)
    {
        sum = sum + *(value + cpt);
    }
    func_average = sum / 10;
    
    return func_average;
}

//fonction de conversion
int convert(float val)
{
    float copy = 0;
    copy = val;
    copy = copy * 100;
    copy = fmod(copy,100);
    if (copy < 26)
    {
        val = floor(val);
    }
    else
    {
        if (copy < 75)
        {
            val = floor(val)+0.5;
        }
        else
        {
            val = ceil(val);
        }
    }
    
    val = val + 50;
    val = val * 2;
    
    return (int)val;
}

int main() 
{
    //Initialisation, détermine les temps de mesure, moyenne et affichage
    sensor.setRanges(4, 400); // min et max en cm
    //pc.printf("Minimum sensor range = %g cm\n\rMaximum sensor range = %g cm\n\r", sensor.getMinRange(), sensor.getMaxRange());
     
     //Initialisation du reveil
    WakeUp::calibrate();
     
     //regle le reveil pour 
    WakeUp::set_ms(2000);
     // Erreur pour BMP180
    
    while(1) {
        
        if (bmp180.init() != 0) {
            //printf("Error communicating with BMP180\n");
        } else {
           // printf("Initialized BMP180\n");
            break;
        }
        wait(1);
    }
    
State state = Start;

    while(1)
    {
        switch(state)
        {
            case Start:
            pc.printf("\n\nSTART\n");
            pc.printf("i =  %d\n",i);
                if(i < 9)
                {
                    state = MesUS;
                    i++;
                }
                else
                {
                    state = MesMeteoBatt;
                    i=0;    
                }
            break;
            
            case DeepSleep:
                pc.printf("DEEPSLEEP \n");
                WakeUp::set(10); //regle le reveil pour 2 min
                deepsleep(); //veille
                state = Start; //au reveil passe a l'etat start
            break;
            
            case SendCarTempPres:
                pc.printf("Envoi donnees\n");
                pc.printf("AT$SS=%02X %02X %02X %02X \r\n",convert(temp), (unsigned int)(pressure/100-900), (unsigned int)batt, car);
                state = DeepSleep;
            break;
            
            case MesMeteoBatt:
                pc.printf("mesure meteo\n");
                bmp180.startTemperature();
                wait_ms(5);     // Wait for conversion to complete
                
                if(bmp180.getTemperature(&temp) != 0)
                {
                    //printf("Error getting temperature\n");
                    continue;
                }
        
                bmp180.startPressure(BMP180::ULTRA_LOW_POWER);
                wait_ms(10);    // Wait for conversion to complete
               
                if(bmp180.getPressure(&pressure) != 0) 
                {
                    continue;
                }
                state = SendCarTempPres;
            break;
            
            case MesUS:
                pc.printf("mesure ultrason\n");
                value = getMesure();
                average = 0;
                average = getAverage();
                pc.printf("US : %f\n",average);
                
                if(average < threshold) // voiture presente
                {
                    car = 1;
                    
                    pc.printf("___voiture presente\n");
                    if(carPresent == false) // changement d'etat
                    {
                        pc.printf("______changement detat\n");
                        carPresent = true;
                        state = MesMeteoBatt;
                    }
                    else // pas de changement
                    {
                        pc.printf("______ pas de changement\n");
                        state = DeepSleep; // 
                    }
                }
                else // voiture non presente
                {
                    car = 0;
                    pc.printf("___voiture non presente\n");
                    if(carPresent == true) // changement d'etat
                    {
                        pc.printf("______changement detat\n");
                        carPresent = false;
                        state = MesMeteoBatt;
                    }
                    else // pas de changement
                    {
                        pc.printf("______ pas de changement\n");
                        state = DeepSleep; // 
                    }
                }
            break;
        }
    }
}
