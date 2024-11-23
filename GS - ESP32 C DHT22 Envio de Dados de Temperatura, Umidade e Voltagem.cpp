#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

// Define o pino do sensor DHT22
const int DHT_PIN = 15;

DHTesp dhtSensor;

// Configurações de Wi-Fi e MQTT
const char* ssid = "Wokwi-GUEST"; // Nome da rede Wi-Fi
const char* password = ""; // Senha da rede Wi-Fi
const char* mqtt_server = "techzap-mqttserver.brazilsouth.cloudapp.azure.com"; // Endereço do servidor MQTT
int port = 1883; // Porta do servidor MQTT
String stMac; // Armazena o endereço MAC do ESP32
char mac[50];
char clientId[50];

// Constantes para cálculo de energia
#define K 5.0 // Constante de proporcionalidade (ajuste conforme necessário)
#define T_MIN 20.0 // Temperatura mínima para geração de energia

// Objetos do cliente Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0; // Armazena o tempo da última mensagem enviada
#define MSG_BUFFER_SIZE (50) // Tamanho do buffer de mensagens
float temp = 0; // Variável para temperatura
float umi = 0; // Variável para umidade

// Função para conectar ao Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.println("Conectando ao WiFi: ");
  Serial.println(ssid);
  Serial.println();

  WiFi.mode(WIFI_STA); // Configura o modo Wi-Fi do ESP32 como estação
  WiFi.begin(ssid, password); // Inicia a conexão com o Wi-Fi

  // Tenta conectar ao Wi-Fi até obter sucesso
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Falha ao conectar!");
  }

  randomSeed(micros()); // Gera uma semente aleatória para o cliente MQTT

  // Exibe informações de conexão
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  stMac = WiFi.macAddress(); // Obtém o endereço MAC do dispositivo
  stMac.replace(":", "_"); // Substitui os ':' por '_' no endereço MAC
  Serial.println(stMac); // Exibe o endereço MAC
}

// Função de callback chamada quando uma mensagem MQTT é recebida
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Mensagem enviada [");
  Serial.println(topic);
  Serial.println("]");
  Serial.println();

  // Imprime o payload recebido
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Controle simples para alterar o estado do DHT_PIN
  if ((char)payload[0] == '1') {
    digitalWrite(DHT_PIN, LOW); // Desliga o sensor
  } else {
    digitalWrite(DHT_PIN, HIGH); // Liga o sensor
  }
}

// Função para reconectar ao broker MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Tentando a conexão com o MQTT");

    // Gera um ID de cliente aleatório
    String clientId = "ESP32CP2RM99466Client-";
    clientId += String(random(0xffff), HEX);

    // Tenta conectar ao broker MQTT
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado com sucesso!");
      client.publish("techzap-appsol/mqtt", "techzap-appsol"); // Publica uma mensagem inicial
      client.subscribe("techzap-appsol/mqtt"); // Subscreve ao tópico desejado
    } else {
      // Em caso de falha, imprime o código de erro e aguarda 5 segundos
      Serial.print("Falha, rc= ");
      Serial.print(client.state());
      Serial.println();
      Serial.println("Tentando novamente em 5 segundos.");
      delay(5000);
    }
  }
}

// Configuração inicial do ESP32
void setup() {
  Serial.begin(115200); // Inicia a comunicação serial
  setup_wifi(); // Conecta ao Wi-Fi
  client.setServer(mqtt_server, port); // Define o servidor MQTT
  client.setCallback(callback); // Define a função de callback
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22); // Configura o sensor DHT22
}

// Loop principal do ESP32
void loop() {
  if (!client.connected()) {
    reconnect(); // Reconecta ao broker se desconectado
  }
  client.loop(); // Mantém a conexão MQTT ativa

  unsigned long now = millis(); // Obtém o tempo atual
  if (now - lastMsg > 2000) { // Verifica se 2 segundos se passaram
    lastMsg = now;
    TempAndHumidity data = dhtSensor.getTempAndHumidity(); // Lê os dados do sensor

    // Lê a temperatura
    float temperature = data.temperature;
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println();
    client.publish("techzap-appsol/temperature", String(temperature, 2).c_str());

    // Cálculo da energia gerada
    float energy = K * (temperature - T_MIN);
    Serial.print("Energy generated: ");
    Serial.print(energy);
    Serial.println(" W");
    client.publish("techzap-appsol/energy", String(energy, 2).c_str());

    // Lê a umidade
    float humidity = data.humidity;
    Serial.print("Umidade: ");
    Serial.print(humidity);
    Serial.println();
    client.publish("techzap-appsol/humidity", String(humidity, 2).c_str());
  }
}