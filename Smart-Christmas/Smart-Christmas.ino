/*
   Smart-Christmas.ino
   Emanuele Signoretta - Ottobre 2020
   Basato su un'idea di Adafruit.com - https://github.com/adafruit/Adafruit_MQTT_Library
*/
#include <SPI.h>
#include <Fishino.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <EEPROM.h>

#define output  9
#define feedback_out 8
#define feedback_in 7

///////////////////////////////////////////////////////////////////////
//           CONFIGURATION DATA -- ADAPT TO YOUR NETWORK             //
//     CONFIGURAZIONE SKETCH -- ADATTARE ALLA PROPRIA RETE WiFi      //
#ifndef __MY_NETWORK_H

// OPERATION MODE :
// NORMAL (STATION)  -- NEEDS AN ACCESS POINT/ROUTER
// STANDALONE (AP)  -- BUILDS THE WIFI INFRASTRUCTURE ON FISHINO
// COMMENT OR UNCOMMENT FOLLOWING #define DEPENDING ON MODE YOU WANT
// MODO DI OPERAZIONE :
// NORMAL (STATION) -- HA BISOGNO DI UNA RETE WIFI ESISTENTE A CUI CONNETTERSI
// STANDALONE (AP)  -- REALIZZA UNA RETE WIFI SUL FISHINO
// COMMENTARE O DE-COMMENTARE LA #define SEGUENTE A SECONDA DELLA MODALITÀ RICHIESTA
//#define STANDALONE_MODE

// here put SSID of your network
// inserire qui lo SSID della rete WiFi
#define MY_SSID  ""

// here put PASSWORD of your network. Use "" if none
// inserire qui la PASSWORD della rete WiFi -- Usare "" se la rete non ￨ protetta
#define MY_PASS ""

// here put required IP address (and maybe gateway and netmask!) of your Fishino
// comment out this lines if you want AUTO IP (dhcp)
// NOTE : if you use auto IP you must find it somehow !
// inserire qui l'IP desiderato ed eventualmente gateway e netmask per il fishino
// commentare le linee sotto se si vuole l'IP automatico
// nota : se si utilizza l'IP automatico, occorre un metodo per trovarlo !
#define IPADDR  192, 168,   1, 125
#define GATEWAY 192, 168,   1, 1
#define NETMASK 255, 255, 255, 0

#endif
//                    END OF CONFIGURATION DATA                      //
//                       FINE CONFIGURAZIONE                         //
///////////////////////////////////////////////////////////////////////

// define ip address if required
// NOTE : if your network is not of type 255.255.255.0 or your gateway is not xx.xx.xx.1
// you should set also both netmask and gateway
#ifdef IPADDR
IPAddress ip(IPADDR);
#ifdef GATEWAY
IPAddress gw(GATEWAY);
#else
IPAddress gw(ip[0], ip[1], ip[2], 1);
#endif
#ifdef NETMASK
IPAddress nm(NETMASK);
#else
IPAddress nm(255, 255, 255, 0);
#endif
#endif


#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // usare la 8883 se si usa il Secure Sockets Layer (SSL)
#define AIO_USERNAME  ""
#define AIO_KEY       ""



//Crea un client appartnente alla classe FishinoClient per la connessione al server MQTT
FishinoClient client;
// oppure usa un FishinoSecureClient per la connessione in SSL
//FishinoSecureClient client;

// Crea il client MQTT utilizzando il FishinoClient, i dati del server e le credenziali di accesso.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
// Crea un feed chiamato 'accendi-gli-addobbi-natalizi'. Verrà utilizzato per ricevere dal server gli aggiornamenti inerenti lo stato.
Adafruit_MQTT_Subscribe addobbionFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/accendi-gli-addobbi-natalizi");
// Crea un feed chiamato 'spegni-gli-addobbi-natalizi'. Verrà utilizzato per ricevere dal server gli aggiornamenti inerenti lo stato.
Adafruit_MQTT_Subscribe addobbioffFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/spegni-gli-addobbi-natalizi");
/*************************** Sketch Code ************************************/


void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(F("Smart-Christmas"));
  pinMode(output, OUTPUT);
  pinMode(feedback_out, OUTPUT);
  pinMode(feedback_in, INPUT);

  int y;
  EEPROM.get(0, y);
  if (y == 0) {
    digitalWrite(output, LOW);
    digitalWrite(feedback_out, LOW);

    Serial.println("Mantengo le luci spente");
  }
  else if (y == 1) {
    digitalWrite(output, HIGH);
    digitalWrite(feedback_out, HIGH);
    Serial.println("Mantengo le luci accese");

  }

  // reset and test wifi module
  // resetta e testa il modulo WiFi
  Serial << F("Resetto il Fishino...");
  while (!Fishino.reset())
  {
    Serial << ".";
    delay(500);
  }
  Serial << F("OK\r\n");


  // set PHY mode to 11G
  Fishino.setPhyMode(PHY_MODE_11G);


  // setup STATION mode
  // imposta la modalitè STATION
  Serial << F("Imposto la modalita' STATION_MODE\r\n");
  Fishino.setMode(STATION_MODE);

  // NOTE : INSERT HERE YOUR WIFI CONNECTION PARAMETERS !!!!!!
  Serial << F("Connessione all' AP...");
  while (!Fishino.begin(MY_SSID, MY_PASS))
  {
    Serial << ".";
    delay(500);
  }
  Serial << F("OK\r\n");

  // setup IP or start DHCP server
#ifdef IPADDR
  Fishino.config(ip, gw, nm);
#else
  Fishino.staStartDHCP();
#endif

  // wait for connection completion
  Serial << F("Attendo l'indirizzo IP...");
  while (Fishino.status() != STATION_GOT_IP)
  {
    Serial << ".";
    delay(500);
  }
  Serial << F("OK\r\n");


  // Inizializza le iscrizoni ai feed MQTT.
  mqtt.subscribe(&addobbionFeed);
  mqtt.subscribe(&addobbioffFeed);
}

uint32_t x = 0;

void loop() {

  // Controlla che la connessione al server MQTT sia attiva(effettuerà la prima connessione
  // e si riconnetterà in automatico quando disconnesso). Controlla la definizione
  // della funzione funzione MQTT_connect di seguito.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;

  while ((subscription = mqtt.readSubscription(5000))) {

    if (subscription == &addobbionFeed) {
      Serial.print(F("Ricevuto: "));
      Serial.println((char *)addobbionFeed.lastread);

      if (digitalRead(feedback_in) == LOW) {
        digitalWrite(feedback_out, HIGH);
        digitalWrite(output, HIGH);
        int z  = 1;
        EEPROM.put(0, z);
        Serial.println("Luci accese");
      }
      else if (digitalRead(feedback_in) == HIGH)
      {
        Serial.println("Luci gia' accese");
      }
    }

    else   if (subscription == &addobbioffFeed) {
      Serial.print(F("Ricevuto: "));
      Serial.println((char *)addobbioffFeed.lastread);

      if (digitalRead(feedback_in) == HIGH) {
        digitalWrite(feedback_out, LOW);
        digitalWrite(output, LOW);
        int z  = 0;
        EEPROM.put(0, z);
        Serial.println("Luci spente");
      }
      else if (digitalRead(feedback_in) == LOW)
      {
        Serial.println("Luci gia' spente");
      }
    }
  }
  // Pinga il server per mantenere attiva la connessione
  if (! mqtt.ping()) {
    mqtt.disconnect();
  }
}


void MQTT_connect() {
  int8_t ret;

  // Stop se già connesso.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connessione al server MQTT io.adafruit.com... ");


  while ((ret = mqtt.connect()) != 0) { // La funzione mqtt.connect restituisce il valore 0 se connesso
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Ritento la connessione al server MQTT tra 5 secondi...");
    mqtt.disconnect();
    delay(5000);  // attende 5 secondi

  }
  Serial.println("MQTT Connesso!");
}
