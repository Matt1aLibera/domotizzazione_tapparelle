#include <Arduino.h>

// Definiamo i pin
#define LED_R    7
#define LED_G    8
#define LED_B    6

// Prototipi delle funzioni Task (i "lavoratori")
void TaskRosso(void *pvParameters);
void TaskVerde(void *pvParameters);
void TaskBlu(void *pvParameters);

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    Serial.println("--- AVVIO TASK FREERTOS ---");

    // Creiamo i 3 Task (i tre processi paralleli)
    // xTaskCreate(Funzione, "Nome", Stack, Parametro, Priorità, Handle)
    xTaskCreate(TaskRosso, "Rosso", 2048, NULL, 1, NULL);
    xTaskCreate(TaskVerde, "Verde", 2048, NULL, 1, NULL);
    xTaskCreate(TaskBlu,   "Blu",   2048, NULL, 1, NULL);
}

void loop() {
    // In FreeRTOS il loop() può rimanere vuoto o essere usato per altro.
    // I task girano da soli "fuori" dal loop.
    delay(1000); 
}

// --- DEFINIZIONE DEI TASK ---

void TaskRosso(void *pvParameters) {
    for (;;) { // Un task non deve mai uscire, quindi usiamo un loop infinito
        digitalWrite(LED_R, HIGH);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Aspetta 1 secondo (non blocca gli altri!)
        digitalWrite(LED_R, LOW);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void TaskVerde(void *pvParameters) {
    for (;;) {
        digitalWrite(LED_G, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS); // Il verde lampeggia più veloce!
        digitalWrite(LED_G, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void TaskBlu(void *pvParameters) {
    for (;;) {
        digitalWrite(LED_B, HIGH);
        vTaskDelay(1500 / portTICK_PERIOD_MS); // Il blu è il più lento
        digitalWrite(LED_B, LOW);
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }
}