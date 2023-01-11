#include <MFRC522.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>

#define SDA_PIN 10
#define RST_PIN 9

MFRC522 Card(SDA_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

const byte ROWS = 4;
const byte COLS = 3;

char hexaKeys[ROWS][COLS] = {
{'1','2','3'},
{'4','5','6'},
{'7','8','9'},
{'*','0','#'}
};
byte rowPins[ROWS] = {0, 1, 2, 3};
byte colPins[COLS] = {4, 5, 6};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27,16,2);
bool Visto = false;
void Menu(){
  String texto1 = "[1] Validar titulo";
  String texto2 = "[2] Adicionar titulos";
  Serial.println(texto1);
  Serial.println(texto2);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(texto1);
  lcd.setCursor(0,1);
  lcd.print(texto2);
}
void Metro(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("Metro");
  lcd.setCursor(5, 1);
  lcd.print("RFID");
}
void RetireCartao(int seg){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("Retire");
  lcd.setCursor(5, 1);
  lcd.print("cartao");

  delay(seg * 1000);
}

/*
 * | Porta |   Cor    | Indicativo |
 * |-------|----------|------------|
 * |   7   | Verde    |  Sucesso   |
 * |   8   | Vermelho |  Insucesso |
*/
void LedInfo(int port,int time){
  if(time>=1){
    digitalWrite(port,1);
    delay(200);
    digitalWrite(port,0);
    delay(200);
    digitalWrite(port,1);
    delay(200);
    digitalWrite(port,0);
    LedInfo(port,time-1);
  }else{
    return;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Testing Serial"));

  lcd.init();
  lcd.backlight();

  Metro();

  SPI.begin();
  Card.PCD_Init();

  pinMode(7,OUTPUT);digitalWrite(7,0);
  pinMode(8,OUTPUT);digitalWrite(8,0);
  delay(2 * 1000);
}

void loop() {
  for(byte i = 0;i<6;i++) key.keyByte[i] = 0xFF;

  if(!Card.PICC_IsNewCardPresent()){
    return;
  }
  if(!Card.PICC_ReadCardSerial()){
    return;
  }
  if(!Visto){
    Menu();
    Visto=true;
  }
  Serial.setTimeout(20000L);

  char customKey = customKeypad.getKey();
  
  switch(customKey){
    case '1':
      // Validar Titulos
      RFIDCard(1);     
      Visto = false;
      Metro();
      break;
    case '2':
      // Adicionar Titulos
      RFIDCard(2);
      Visto = false;
      Metro();
      break;
  }

}

void RFIDCard(int opcao){
  bool Veri = false;
  switch(opcao){
    case 1:
      // Validar titulo
      Veri = !VerificarRegistro(opcao);
      if(Veri){
        // Código Verificação Incorreto
        Serial.println(F("Não validado"));
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Cartao Invalido");
        lcd.setCursor(0,1);
        lcd.print("Adicione Titulos");

        LedInfo(8,2);

        delay(2.5 * 1000);
      }else{
        // Código Verificação Correto
        SubtrairTitulos();
      }
      break;
    case 2:
      // Adicionar titulos
      VerificarRegistro(opcao);
      AdicionarTitulos();
      break;
    default:
      Serial.println(F("Erro"));
      break;
  }
  
  Card.PICC_HaltA();
  Card.PCD_StopCrypto1();
  
}

/*
 * Função para escrever o código de Verificação da aplicação
*/
void EscreverVerificacao(byte block){
  Serial.println(F("A executar EscreverVerificacao"));
  byte verificationCode[] = {
    0x34, 0x31, 0x39, 0x38,
    0x37, 0x2d, 0x49, 0x45,
    0x45, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20
  };
  status = Card.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(Card.uid));
	
  if(status != MFRC522::STATUS_OK){
    Serial.print(F("Autenticacao falhou: "));
    Serial.println(Card.GetStatusCodeName(status));
    LedInfo(8,1);
    return;
  }

  status = Card.MIFARE_Write(block, verificationCode, 16);
  if(status != MFRC522::STATUS_OK){
    Serial.print(F("Escrita falhou:"));
    Serial.println(Card.GetStatusCodeName(status));
    LedInfo(8,1);
    return;
  }else Serial.println(F("Código de Verificação escrito com sucesso"));  

}
/*
 * Função para verificar se o cartão está registro dentro da aplicação
*/
bool VerificarRegistro(int opcao){
  bool Checker = true;
  /*
   * Código de verificação:
   * 41987-IEE
   * 34 31 39 38  37 2D 49 45  45 20 20 20  20 20 20 20
   */
  
  byte buffer[18];
  byte CheckBuffer[] = {
    0x34, 0x31, 0x39, 0x38,
    0x37, 0x2d, 0x49, 0x45,
    0x45, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20  
  };
  byte block = 1;
  byte len = 18;

  status = Card.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(Card.uid));
  if(status != MFRC522::STATUS_OK){
    Serial.print(F("Autenticacao falhou: "));
    Serial.println(Card.GetStatusCodeName(status));
    LedInfo(8,1);
    return;
  }

  status = Card.MIFARE_Read(block, buffer, &len);
  if(status != MFRC522::STATUS_OK){
    Serial.print(F("Leitura falhou:"));
    Serial.println(Card.GetStatusCodeName(status));
    LedInfo(8,1);
    return;
  }

  // Verificação de Igualdade
  for(byte i = 0;i<16;i++){
    if(buffer[i] != CheckBuffer[i]){
        Checker = false;
    }
  }
  if(!Checker && opcao == 2) EscreverVerificacao(block);

  return Checker;
}

/*
 * Função que retorna o número de titulos
 * disponiveis no cartão do usuário
 * A informação do número de titulos fica disponivel
 * no bloco 4 da memoria do cartão
*/
int LerTitulos(){
  byte block = 4;
  byte len = 18;
  byte buffer[18];
  String titlesText;
  int numberTitles;
  
  status = Card.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(Card.uid));
  if(status != MFRC522::STATUS_OK){
    Serial.print(F("Autenticacao falhou: "));
    Serial.println(Card.GetStatusCodeName(status));
    LedInfo(8,1);
    return;
  }

  status = Card.MIFARE_Read(block, buffer, &len);
  if(status != MFRC522::STATUS_OK){
    Serial.print(F("Leitura falhou:"));
    Serial.println(Card.GetStatusCodeName(status));
    LedInfo(8,1);
    return;
  }
  titlesText = String((char*)buffer);
  numberTitles = texto.toInt();

  return numberTitles;
}
/*
 * Função para escrever o número de titulos
 * disponiveis no cartão do usuário
*/
void EscreverTitulos(byte* block,byte buffer[]){
  status = Card.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (*block), &key, &(Card.uid));
  if(status != MFRC522::STATUS_OK){
    Serial.print(F("Autenticacao falhou: "));
    Serial.println(Card.GetStatusCodeName(status));
    LedInfo(8,1);
    return;
  }

  status = Card.MIFARE_Write((*block), buffer, 16);
  if(status != MFRC522::STATUS_OK){
    Serial.print(F("Escrita falhou:"));
    Serial.println(Card.GetStatusCodeName(status));
    LedInfo(8,1);
    return;
  }else Serial.println(F("Titulo Validado com sucesso"));
}
/*
 * Função para validar o titulo do usuário
 * caso existam titulos disponiveis
*/
void SubtrairTitulos(){
  byte block = 4;
  byte len = 18;
  byte buffer[16];
  String titlesText;
  int numberTitles;

  numberTitles = LerTitulos();

  if(numberTitles<1){
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("Sem Titulos");

    LedInfo(8,2);

    delay(2.5 * 1000);
    RetireCartao(2);
    return;
  }else{
    numberTitles = numberTitles - 1;

    Serial.print(F("Numero de titulos:")); Serial.println(numberTitles);

    titlesText = String(numberTitles);

    titlesText.getBytes(buffer,16);

    EscreverTitulos(&block, buffer);
    Serial.println("");Serial.println("");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("N titulos: ");lcd.print(numberTitles);
    lcd.setCursor(0,1);
    lcd.print("Validado");

    LedInfo(7,2);

    delay(2.5 * 1000);

    RetireCartao(2);
  }
}
void AdicionarTitulos(){
  bool Runner = true;
  int numberTitles = LerTitulos();
  int NovosTitulos;
  byte buffer[16];
  byte block = 4;

  char caract[3];
  int count = 0;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Insira o numero");
  lcd.setCursor(0,1);
  lcd.print("# para terminar");

  Serial.print(F("N de titulos:"));
  char key = customKeypad.getKey();
  while(Runner){
    if(key != NO_KEY){
      if(key == '#'){   
        Runner = false;
      }else{
        if(!(key == '*')){
          Serial.print(key);
          caract[count] = key;
          count++;
        }     
      }     
    }
    if(count == 3){
      break;
    }
    key = customKeypad.getKey();
  }

  String titlesText;
  for(int i = 0; i<count;i++){
    //Serial.println(caract[i]);
    titlesText += caract[i];
  }
  
  titlesText = String( titlesText.toInt() + numberTitles);

  
  Serial.println("");
  Serial.print(F("Titulos:")); Serial.println(titlesText);
  titlesText.getBytes(buffer,16);

  EscreverTitulos(&block,buffer);
  
  Serial.println("");Serial.println("");


  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("N titulos: ");lcd.print(titlesText.toInt());
  lcd.setCursor(0,1);
  lcd.print("Adicionado Ok");

  LedInfo(7,1);

  delay(2.5 * 1000);
  
  RetireCartao(2);
}