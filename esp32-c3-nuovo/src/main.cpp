#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>


// --- CONFIGURAZIONE WIFI---
const char* ssid     = "SpotHotGirl";
const char* password = "giorgio1234";

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "788c0543b50345e1911757c4b40d3448.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "MattiaLibera";
const char* mqtt_pass = "Aaa.ask.fm1!";

// Definiamo i pin
#define PIN_SALI    7
#define PIN_SCENDI  8

// Oggetti per la connessione
WiFiClientSecure espClient;
PubSubClient client(espClient); //Creaun istanza di client parzialmente inizializzata, prima di esere usata bisogna darle i dettagli del server, viene fatto nel setup

// --- RISORSE FREERTOS ---
// Coda per scambiare messaggi tra i Task
// 0 = STOP, 1 = SALI, 2 = SCENDI
QueueHandle_t codaComandi;  //In C, quando crei qualcosa di complesso come un file o una coda, il sistema operativo ti dà un "Handle" (una maniglia), non l'oggetto intero
                            //Teoria: È un puntatore a una struttura dati interna di FreeRTOS che contiene tutte le info sulla coda (dove si trova in memoria, quanti messaggi ha, chi sta aspettando).

// Prototipi delle funzioni Task (i "lavoratori")
void TaskMQTT(void *pvParameters);
void TaskWiFi(void *pvParameters);
void TaskMotore(void *pvParameters);
//void TaskVerde(void *pvParameters);
void callback(char* topic, byte* payload, unsigned int length); //funzione di ricezione dei messaggi

void setup() {
    Serial.begin(115200);

    // Creazione della CODA (può contenere 10 interi), null se fallisce
    codaComandi = xQueueCreate(10, sizeof(int));

    // Configurazione Sicurezza per HiveMQ (Ignora controllo certificato per ora)
    espClient.setInsecure();

    Serial.println("--- AVVIO TASK FREERTOS ---");
    if (codaComandi != NULL) { //crea i task solo se coda è stata creata correttamente, se no non vale neanche la pena crearli 
        // Creazione Task
        xTaskCreate(TaskWiFi,       "WiFi_Task",   4096, NULL, 1, NULL);
        //xTaskCreate(TaskMotore,     "Motor_Task",  2048, NULL, 2, NULL);
        //xTaskCreate(TaskSimulatore, "Sim_Task",    2048, NULL, 1, NULL);
    }
    
    // Sicurezza: diciamo all'ESP32 di accettare la connessione SSL senza verificare il certificato CA
    espClient.setInsecure(); 

    client.setServer(mqtt_server, mqtt_port);  //passiamo i dettagli del server al client, client pronto per essere usato
    client.setCallback(callback); // Collega la funzione callback scritta sopra

    Serial.println("Setup completato. Sistema RTOS pronto.");

    // Creiamo i 3 Task (i tre processi paralleli)
    // xTaskCreate(Funzione, "Nome", Stack, Parametro, Priorità, Handle)

}

void loop() {
    // In FreeRTOS il loop() può rimanere vuoto o essere usato per altro.
    // I task girano da soli "fuori" dal loop.
    delay(1000); 
}

// --- DEFINIZIONE DEI TASK ---

// Task che gestisce la connessione Wi-Fi e mqtt
void TaskWiFi(void *pvParameters) {
    Serial.println("Task WiFi: Avviato");
    int connected=0;
    WiFi.begin(ssid, password); //funzione non bloccante. Dice al chip radio dell'ESP32 di cercare la rete".
                                     //L'ESP32 gestisce la connessione in un "sotto-task" hardware nascosto.
    while (true) {
        if (WiFi.status() == WL_CONNECTED) {
            if(connected == 0){
                connected=1;
                Serial.println("connesso su: " + WiFi.localIP().toString());// Se connesso, stampa l'IP solo 1 volta
            }
            // Serial.print("IP: ");
            //vTaskDelay(pdMS_TO_TICKS(5000)); //gestione rozza dello sleep, da approfondire

            if(!client.connected()){//forse qui sarebbe meglio usare client.loop? e connected si usa magari subito prima di inviare un mex? bho non so i best usage
                //se non connesi a mqtt proviamo a connetterci
                if(client.connect("ESP32-c3", mqtt_user, mqtt_pass)){
                    Serial.print("connesso al server mqtt");
                    //possiamo inviare e ricevere messaggi ma dobbiamo iscriverci
                    client.subscribe("ML/tapparelle/prova"); //se arrivano messaggi su questo topic mi avvisa?

                    if(client.publish("ML/tapparelle/connesso", "connesso")){
                        Serial.print("messaggio inviato al server mqtt");
                    }
                }else{//non siamo riusciti a connetterci, riproviamo tra poco
                    Serial.print("errore MQTT, code:  ");
                    Serial.print(client.state());
                    vTaskDelay(pdMS_TO_TICKS(5000)); //aspetto 5 sec prima di riconnettermi
                }
            }
            //se siamo connesi al server mqtt cerchiamo di rimanere connessi
            if(!client.loop()){//per adesso non hotrovato un uso migliore se non capire se sono ancor aconnesso o no oltre a fare un keepalive
               Serial.print("connessione mqtt persa");
            }
        } else {
            if(connected == 1){
                connected=0;
            }
            Serial.println("wifi perso, connessione in corso...");
            WiFi.begin(ssid, password);
            vTaskDelay(pdMS_TO_TICKS(10000)); // Aspetta 10 sec prima di riprovare,//gestione rozza dello sleep, da approfondire
        }

        // FONDAMENTALE: Una piccola pausa per far respirare il Watchdog di FreeRTOS
        // Evita che il task monopolizzi il processore
        vTaskDelay(pdMS_TO_TICKS(10));//sempre grezzo
    }
}

// 2. Task che gestisce i Relè
/*void TaskMotore(void *pvParameters) {
    pinMode(PIN_SALI, OUTPUT);
    pinMode(PIN_SCENDI, OUTPUT);
    // Relè spenti all'inizio (Active Low: HIGH = spento)
    digitalWrite(PIN_SALI, HIGH);
    digitalWrite(PIN_SCENDI, HIGH);

    int comandoRicevuto;

    for (;;) {
        // Aspetta un comando dalla coda (si blocca qui senza consumare CPU)
        if (xQueueReceive(codaComandi, &comandoRicevuto, portMAX_DELAY)) {
            
            if (comandoRicevuto == 1) { // Esempio: 1 = SALI
                Serial.println("MOTORE: Eseguo SALITA...");
                digitalWrite(PIN_SALI, LOW); 
                vTaskDelay(pdMS_TO_TICKS(3000)); // Simula movimento per 3 sec
                digitalWrite(PIN_SALI, HIGH);
                Serial.println("MOTORE: Fine Salita.");
            } 
            else if (comandoRicevuto == 2) { // Esempio: 2 = SCENDI
                Serial.println("MOTORE: Eseguo DISCESA...");
                digitalWrite(PIN_SCENDI, LOW);
                vTaskDelay(pdMS_TO_TICKS(3000));
                digitalWrite(PIN_SCENDI, HIGH);
                Serial.println("MOTORE: Fine Discesa.");
            }
        }
    }
}

// 3. Task Temporaneo per testare il sistema (Simula un server o un pulsante)
void TaskSimulatore(void *pvParameters) {
    int prossimoComando = 1; // Inizia con Salita
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(15000)); // Aspetta 15 secondi tra un test e l'altro
        
        Serial.print("SIMULATORE: Invio comando alla coda: ");
        Serial.println(prossimoComando);
        
        xQueueSend(codaComandi, &prossimoComando, portMAX_DELAY);
        
        // Alterna tra 1 e 2
        prossimoComando = (prossimoComando == 1) ? 2 : 1;
    }
}
    */

/*void TaskMotore(void *pvParameters) {
    // Inizialmente mettiamo i pin in INPUT per tenerli spenti
    pinMode(PIN_SALI, INPUT);
    pinMode(PIN_SCENDI, INPUT);

    for (;;) {
        // --- SALITA ---
        Serial.println("Accendo SALITA...");
        pinMode(PIN_SALI, OUTPUT);
        digitalWrite(PIN_SALI, LOW); // Accende
        vTaskDelay(10000 / portTICK_PERIOD_MS); 

        // --- SPEGNIMENTO ---
        Serial.println("Spengo tutto (Pausa 5s)...");
        pinMode(PIN_SALI, INPUT);    // "Stacca" il pin
        pinMode(PIN_SCENDI, INPUT);  // "Stacca" il pin
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        // --- DISCESA ---
        Serial.println("Accendo DISCESA...");
        pinMode(PIN_SCENDI, OUTPUT);
        digitalWrite(PIN_SCENDI, LOW); // Accende
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        // --- SPEGNIMENTO ---
        Serial.println("Spengo tutto (Pausa 5s)...");
        pinMode(PIN_SALI, INPUT);
        pinMode(PIN_SCENDI, INPUT);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}*/
//i messaggi MQTT arrivano come un "treno" di singoli byte che devi ricostruire.
/*
    - char* topic: È il nome del canale da cui arriva il messaggio (es. "ML/tapparelle/prova"). Usiamo il puntatore (*) perché è una stringa di testo
    - byte* payload: Questo è il contenuto del messaggio. È un array di byte puri. MQTT non sa se stai mandando una foto, un numero o del testo; lui vede solo bit.
    - unsigned int length: Ti dice quanto è lungo il messaggio (quanti byte compongono il payload).ù
        la callback "interrompe" il flusso normale, ci sono due regole ferree da seguire:
            1. Deve essere velocissima: Non mettere mai dei delay() o dei cicli infiniti dentro la callback
            2. Non pilotare direttamente l'hardware pesante: Invece di scrivere digitalWrite(RELÈ, HIGH) qui dentro, la cosa professionale da fare è immettere il messaggio in coda
*/
void callback(char* topic, byte* payload, unsigned int length){ //funzione usata nel momento in cui si riceve un messaggio
    Serial.print("Messaggio ricevuto sul topic: ");
    Serial.println(topic);

    String messaggio;
    for (int i = 0; i < length; i++){//Ricostruire il messaggio
        messaggio += (char)payload[i];//casting. Prende il numero (es. 83) e lo trasforma nel carattere corrispondente (es. 'S')
    }
    Serial.println("Contenuto: " + messaggio);
}


/*void TaskVerde(void *pvParameters) {
    for (;;) {
        digitalWrite(LED_G, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS); // Il verde lampeggia più veloce!
        digitalWrite(LED_G, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
*/