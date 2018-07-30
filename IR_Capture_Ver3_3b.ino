/* 赤外線を読み取る
   Date 2012/01/18
   NECフォーマットのみ対応
   再送については考慮してないが、問題ないっぽい
   
   どうも他のキー操作が受け付けないので全て入力端子に設定するように変更
   Serialが動いてるとダメっぽい。使用しているシリアルをすべて
   //S でコメントアウトする

*/

//### ピンアサイン ###
const int ir_in = 3;          //赤外線入力
const int pin_btnin = 4;      //押釦入力
const int pin_power = 2;      //電源ボタン
const int pin_under = 0;      //↓ボタン
const int pin_upper = 1;      //↑ボタン
const int pin_timer = 18;     //タイマーボタン
const int pin_addtimer = 19;  //3時間延長ボタン

//### NECフォーマットの規定 ###
unsigned int ir_reader_on  = 9000;    //9000us の1割引き
unsigned int ir_reader_off = 4500;    //4500us の1割引き
unsigned int ir_pulse_on   = 560;     //560us  の1割引き
unsigned int ir_pulse_1    = 1690;    //2250us - 560us の1割引き
unsigned int ir_pulse_0    = 565;     //1125us - 560us の1割引き

//### 変数定義 ###
unsigned long us = micros();  //経過時間
int ir_last = 0;     //前回結果  0がOFF 1がON状態    1の長さはリーダー以外一定のはず
int ir_databit = 0;  //unsig_int の中のビット位置
int ir_datafigure = 0; //データの桁 data[ir_datafigure]

//赤外線データ
unsigned int data[10] = {};  //データ

// 0.電源(順送り) 1.上 2.下 3.30分延長(30分タイマー) 4.タイマー(タイマー解除)
//各出力状態
boolean powerSW = 1;
boolean upperSW = 0;
boolean underSW = 0;
boolean addtimerSW = 0;
boolean timerSW = 0;

//ターゲットデータ
const unsigned int target0[10] = {0x405D,0x8877,0x9E73,0xE01,0xF400};
const unsigned int target1[10] = {0x405D,0x9867,0x8,0x0,0x0};
const unsigned int target2[10] = {0x405D,0xA857,0x8,0x0,0x0};
const unsigned int target3[10] = {0x405D,0xA05F,0x9E73,0xE81,0x7400};
const unsigned int target4[10] = {0x405D,0xB04F,0x9E73,0xF00,0xF400};


//セットアップ
void setup(){
//S  Serial.begin(57600);   //シリアル転送設定
  pinMode(ir_in, INPUT);  //赤外線入力PIN
  pinMode(pin_btnin, INPUT);    //ボタン入力
  pinMode(pin_power , OUTPUT);
  pinMode(pin_under , OUTPUT);   //シリアルと競合
  pinMode(pin_upper , OUTPUT);   //シリアルと競合
  pinMode(pin_timer , INPUT);
  pinMode(pin_addtimer , INPUT);
  pinMode(13 , OUTPUT);          //LED
  powerSW = 0;
  digitalWrite(ir_in , LOW);    //プルアップを無効
  digitalWrite(pin_btnin , LOW);
  digitalWrite(pin_power , LOW);
  digitalWrite(pin_under , LOW);
  digitalWrite(pin_upper , LOW);
  digitalWrite(pin_timer , LOW);
}


void loop(){
  unsigned int val;  //赤外線状態
  unsigned int timeout = 300000000;  //30msでタイムアウト
  
//赤外線状態を読む
  //パルスが切り替わるまで待機
  while( (val = digitalRead(ir_in)) == ir_last){
    if( 0 == --timeout){  //タイムアウト
      ir_datafigure = 0;
      ir_databit = 0;
//S    Serial.print("JDbefore sw_state = ");
//S    Serial.println(powerSW);
    
    //受信したコードと比較して ピン出力変更
     powerSW    = jadgement(target0, pin_power, powerSW);       //電源
     upperSW    = jadgement2(target1, pin_upper, upperSW);      //上
     underSW    = jadgement2(target2, pin_under, underSW);      //下
     addtimerSW = jadgement2(target3, pin_addtimer, addtimerSW); //30分延長
     timerSW    = jadgement2(target4, pin_timer, timerSW);      //タイマー
     

//S    Serial.print("JDafter sw_state = ");
//S    Serial.println(powerSW);

      
//S      Serial.println("timeout");
        //結果をシリアルポートに出力
        for(int i=0;i< 10;i++){
//S          Serial.print(data[i], HEX );
//S          Serial.print(",");
          data[i] = 0;//出力しながら初期化
        }
      break;
    }
  }
  unsigned long ir_us = micros() - us;
  us = micros();

  //パルスが切り替わった！
  if( 0 < timeout && val == 0){       //タイムアウトトリガーではなく、信号が立下りの時
    //桁をずらして1bit目に値を入れる作戦
    if( ir_databit < 16 ){
      data[ir_datafigure] <<= 1;  //桁をズラス
    }else{
      ir_databit = 0;   //データ位置指定を初期化
      ir_datafigure++;  //データ桁を++
    }
    //解析
    if( (ir_reader_off * 0.8) < ir_us && ir_us < (ir_reader_off * 1.2) ){
      data[ir_datafigure] |= 1;
    }else if( (ir_pulse_1 * 0.8) < ir_us && ir_us < (ir_pulse_1 * 1.2) ){
      data[ir_datafigure] |= 1;
    }else if( (ir_pulse_0 * 0.8) < ir_us && ir_us < (ir_pulse_0 * 1.2) ){
      data[ir_datafigure] |= 0;
    }
    ir_databit++;
  }
  
  //ボタンが押された系
  if( digitalRead(pin_btnin) == LOW ){
    powerSW = !powerSW;
    digitalWrite(pin_power ,powerSW );
  }
 
 digitalWrite( 13, powerSW);
 
  //現在の赤外線状態を過去に回す
  ir_last = val;
}

//赤外線データを判定する
boolean jadgement(const unsigned int targetdata[], int pin, boolean state){
//  Serial.println("compare START");

  // 比較(Power)
  for( int i=0;i<5;i++){
//    Serial.println(target1[i], HEX);
//    Serial.println(data[i], HEX);
    if(targetdata[i] != data[i]) break;
//    Serial.println("=== PARFECT ===");
    if(i<4) continue;
//S    Serial.println("### NO CONTIUNUE ###");
    state = !state;
    digitalWrite(pin , state );
  }
  
  return state;
}

boolean jadgement2(const unsigned int targetdata[], int pin, boolean state){
//  Serial.println("compare START");

  // 比較(Power)
  for( int i=0;i<5;i++){
//    Serial.println(target1[i], HEX);
//    Serial.println(data[i], HEX);
    if(targetdata[i] != data[i]) break;
//    Serial.println("=== PARFECT ===");
    if(i<4) continue;
//S    Serial.println("### NO CONTIUNUE ###");
    pinMode(pin, OUTPUT);
    state = !state;
    digitalWrite(pin , state );
    delay(300);
    state =!state;
    digitalWrite(pin , state );
    pinMode(pin, INPUT);
  }
  
  return state;
}

