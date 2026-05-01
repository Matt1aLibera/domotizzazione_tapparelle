#include <Arduino.h>
#include <WiFi.h>


// --- CONFIGURAZIONE WIFI---
const char* ssid     = "SpotHotGirl";
const char* password = "giorgio1234";

// Definiamo i pin
#define PIN_SALI    7
#define PIN_SCENDI  8

// --- RISORSE FREERTOS ---
QueueHandle_t codaComandi;  //In C, quando crei qualcosa di complesso come un file o una coda, il sistema operativo ti dà un "Handle" (una maniglia), non l'oggetto intero
                            //Teoria: È un puntatore a una struttura dati interna di FreeRTOS che contiene tutte le info sulla coda (dove si trova in memoria, quanti messaggi ha, chi sta aspettando).

// Prototipi delle funzioni Task (i "lavoratori")
void TaskWiFi(void *pvParameters);
void TaskMotore(void *pvParameters);
//void TaskVerde(void *pvParameters);

void setup() {
    Serial.begin(115200);

    // Creazione della CODA (può contenere 10 interi), null se fallisce
    codaComandi = xQueueCreate(10, sizeof(int));

    Serial.println("--- AVVIO TASK FREERTOS ---");
    if (codaComandi != NULL) { //crea i task solo se coda è stata creata correttamente, se no non vale neanche la pena crearli 
        // Creazione Task
        xTaskCreate(TaskWiFi,       "WiFi_Task",   4096, NULL, 1, NULL);
        xTaskCreate(TaskMotore,     "Motor_Task",  2048, NULL, 2, NULL);
        //xTaskCreate(TaskSimulatore, "Sim_Task",    2048, NULL, 1, NULL);
    }
    
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

// Task che gestisce la connessione Wi-Fi
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
            vTaskDelay(pdMS_TO_TICKS(5000)); //gestione rozza dello sleep, da approfondire
        } else {
            if(connected == 1){
                connected=0;
            }
            Serial.println("Tentativo di connessione in corso...");
            WiFi.begin(ssid, password);
            vTaskDelay(pdMS_TO_TICKS(10000)); // Aspetta 10 sec prima di riprovare,//gestione rozza dello sleep, da approfondire
        }
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

void TaskMotore(void *pvParameters) {
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