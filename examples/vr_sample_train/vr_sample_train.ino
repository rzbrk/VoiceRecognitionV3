/**
  ******************************************************************************
  * @file    vr_sample_train.ino
  * @author  JiapengLi
  * @brief   This file provides a demostration on 
              how to train VoiceRecognitionModule to record your voice
  ******************************************************************************
  * @note:
         Use serial command to control VoiceRecognitionModule. '
         All commands are case insensitive. Default serial baud rate 115200.
         
   COMMAND        FORMAT                        EXAMPLE                    Comment
   
   train          train (r0) (r1)...            train 0 2 14               Train records
   load           load (r0) (r1) ...            load 0 1 2 3               Load records
   clear          clear                         clear                      remove all records in  Recognizer
   record         record / record (r0) (r1)...  record / record 0 79       Check record train status
   vr             vr                            vr                         Check recognizer status
   getsig         getsig (r)                    getsig 0                   Get signature of record (r)
   sigtrain       sigtrain (r) (sig)            sigtrain 0 ZERO            Train one record(r) with signature(sig)
  ******************************************************************************
  * @section  HISTORY
    
    2013/06/13    Initial version.
  */
#include <SoftwareSerial.h>
#include "VoiceRecognitionV2.h"

/**        
  Connection
  Arduino    VoiceRecognitionModule
   2   ------->     TX
   3   ------->     RX
*/
VR myVR(2,3);    // 2:RX 3:TX, you can choose your favourite pins.

/***************************************************************************/
/** declare print functions */
void printSeperator();
void printSignature(uint8_t *buf, int len);
void printVR(uint8_t *buf);
void printLoad(uint8_t *buf, uint8_t len);
void printTrain(uint8_t *buf, uint8_t len);
void printCheckRecognizer(uint8_t *buf);
void printUserGroup(uint8_t *buf, int len);
void printCheckRecord(uint8_t *buf, int num);
void printSigTrain(uint8_t *buf, uint8_t len);

/***************************************************************************/
// command analyze part
#define CMD_BUF_LEN      64+1
#define CMD_NUM      8
typedef int (*cmd_function_t)(int, int);
uint8_t cmd[CMD_BUF_LEN];
uint8_t cmd_cnt;
uint8_t *paraAddr;
int receiveCMD();
int checkCMD(int len);
int checkParaNum(int len);
int findPara(int len, int paraNum, uint8_t **addr);
int compareCMD(uint8_t *para1 , uint8_t *para2, int len);

int cmdTrain(int len, int paraNum);
int cmdLoad(int len, int paraNum);
int cmdTest(int len, int paraNum);
int cmdVR(int len, int paraNum);
int cmdClear(int len, int paraNum);
int cmdRecord(int len, int paraNum);
int cmdSigTrain(int len, int paraNum);
int cmdGetSig(int len, int paraNum);

/** cmdList, cmdLen, cmdFunction has correspondence */
const char cmdList[CMD_NUM][10] = {  // command list table
  {"train"},
  {"load"}, 
  {"clear"},
  {"vr"},
  {"record"},
  {"sigtrain"},
  {"getsig"},
  {"test"},
};
const char cmdLen[CMD_NUM]= {    // command length
  5,  //  {"train"},
  4,  //  {"load"}, 
  5,  //  {"clear"},
  2,  //  {"vr"},
  6,  //  {"record"},
  8,  //  {"sigtrain"},
  6,  //  {"getsig"},
  4,  //  {"test"}
};
cmd_function_t cmdFunction[CMD_NUM]={      // command handle fuction(function pointer table)
  cmdTrain,
  cmdLoad,
  cmdClear,
  cmdVR,
  cmdRecord,
  cmdSigTrain,
  cmdGetSig,
  cmdTest,
};

/***************************************************************************/
/** temprory data */
uint8_t buf[200];
uint8_t records[7]; // save record

void setup(void)
{
  /** initialize */
  Serial.begin(115200);
  Serial.println("Elechouse Voice Recognition V2 Module \"train\" sample.");

  cmd_cnt = 0;
}

void loop(void)
{
  int len, paraNum, paraLen, i;
  
  /** receive Serial command */
  len = receiveCMD();
  if(len>0){
    /** check if the received command is valid */
    if(!checkCMD(len)){
      
      /** check parameter number of the received command  */
      paraNum = checkParaNum(len);
      
      /** display the receved command back */
      Serial.write(cmd, len);
      
      /** find the first parameter */
      paraLen = findPara(len, 1, &paraAddr);
      
      /** compare the received command with command in the list */
      for(i=0; i<CMD_NUM; i++){
        /** compare command length */
        if(paraLen == cmdLen[i]){
          /** compare command content */
          if( compareCMD(paraAddr, (uint8_t *)cmdList[i], paraLen) == 0 ){
            /** call command function */
            if( cmdFunction[i](len, paraNum) != 0){
              Serial.println("Command Format Error!");
            }
            break;
          }
        }
      }
      
      /** command is not supported*/
      if(i == CMD_NUM){ 
          Serial.println("Unkonwn command");
      }
    }else{
      /** received command is invalid */
      Serial.println("Command format error");
    }
  }
  
  /** try to receive recognize result */
  int ret;
  ret = myVR.recognize(buf, 50);
  if(ret>0){
    /** voice recognized, print result */
    printVR(buf);
  }
}

/**
  @brief   receive command from Serial.
  @param   NONE.
  @retval  command length, if no command receive return -1.
*/
int receiveCMD()
{
  int ret;
  int len;
  unsigned long start_millis;
  start_millis = millis();
  while(1){
    ret = Serial.read();
    if(ret>0){
      start_millis = millis();
      cmd[cmd_cnt] = ret;
      if(cmd[cmd_cnt] == '\n'){
        len = cmd_cnt+1;
        cmd_cnt = 0;
        return len;
      }
      cmd_cnt++;
      if(cmd_cnt == CMD_BUF_LEN){
        cmd_cnt = 0;
        return -1;
      }
    }
    
    if(millis() - start_millis > 100){
      cmd_cnt = 0;
      return -1;
    }
  }
}

/**
  @brief   compare two commands, case insensitive.
  @param   para1  -->  command buffer 1
           para2  -->  command buffer 2
           len    -->  buffer length
  @retval  0  --> equal
          -1  --> unequal
*/
int compareCMD(uint8_t *para1 , uint8_t *para2, int len)
{
  int i;
  uint8_t res;
  for(i=0; i<len; i++){
    res = para2[i] - para1[i];
    if(res != 0 && res != 0x20){
      return -1;
    }else{
      res = para1[i] - para2[i];
      if(res != 0 && res != 0x20){
        return -1;
      }
    }
  }
  return 0;
}

/**
  @brief   Check command format.
  @param   len  -->  command length
  @retval  0  -->  command is valid
          -1  -->  command is invalid
*/
int checkCMD(int len)
{
  int i;
  for(i=0; i<len; i++){
    if(cmd[i] > 0x1F && cmd[i] < 0x7F){
    
    }else if(cmd[i] == '\t' || cmd[i] == ' ' || cmd[i] == '\r' || cmd[i] == '\n'){
    
    }else{
      return -1;
    }
  }
  return 0;
}

/**
  @brief   Check the number of parameters in the command
  @param   len  -->  command length
  @retval  number of parameters
*/
int checkParaNum(int len)
{
  int cnt=0, i;
  for(i=0; i<len; ){
    if(cmd[i]!='\t' && cmd[i]!=' ' && cmd[i] != '\r' && cmd[i] != '\n'){
      cnt++;
      while(cmd[i] != '\t' && cmd[i] != ' ' && cmd[i] != '\r' && cmd[i] != '\n'){
        i++;
      }
    }
    i++;
  }
  return cnt;
}

/**
  @brief   Find the specified parameter.
  @param   len       -->  command length
           paraIndex -->  parameter index
           addr      -->  return value. position of the parameter
  @retval  length of specified parameter
*/
int findPara(int len, int paraIndex, uint8_t **addr)
{
  int cnt=0, i, paraLen;
  uint8_t dt;
  for(i=0; i<len; ){
    dt = cmd[i];
    if(dt!='\t' && dt!=' '){
      cnt++;
      if(paraIndex == cnt){
        *addr = cmd+i;
        paraLen = 0;
        while(cmd[i] != '\t' && cmd[i] != ' ' && cmd[i] != '\r' && cmd[i] != '\n'){
          i++;
          paraLen++;
        }
        return paraLen;
      }else{
        while(cmd[i] != '\t' && cmd[i] != ' ' && cmd[i] != '\r' && cmd[i] != '\n'){
          i++;
        }
      }
    }else{
      i++;
    }
  }
  return -1;
}

/**
  @brief   Handle "train" command
  @param   len     --> command length
           paraNum --> number of parameters
  @retval  0 --> success
          -1 --> Command format error
*/
int cmdTrain(int len, int paraNum)
{
  int i, ret;
  if(paraNum < 2 || paraNum > 8 ){
    return -1;
  }
  
  for(i=2; i<=paraNum; i++){
    findPara(len, i, &paraAddr);
    records[i-2] = atoi((char *)paraAddr);
    if(records[i-2] == 0 && *paraAddr != '0'){
      return -1;
    }
  }
  ret = myVR.train(records, paraNum-1, buf);
//  ret = myVR.train(records, paraNum-1);
  if(ret >= 0){
    printTrain(buf, ret);
  }else{
    Serial.println("Train failed.");
  }
  return 0;
}

/**
  @brief   Handle "load" command
  @param   len     --> command length
           paraNum --> number of parameters
  @retval  0 --> success
          -1 --> Command format error
*/
int cmdLoad(int len, int paraNum)
{
  int i, ret;
  if(paraNum < 2 || paraNum > 8 ){
    return -1;
  }
  
  for(i=2; i<=paraNum; i++){
    findPara(len, i, &paraAddr);
    records[i-2] = atoi((char *)paraAddr);
    if(records[i-2] == 0 && *paraAddr != '0'){
      return -1;
    }
  }
//  myVR.writehex(records, paraNum-1);
  ret = myVR.load(records, paraNum-1, buf);
  printSeperator();
  if(ret >= 0){
    printLoad(buf, ret);
  }else{
    Serial.println("Load failed.");
  }
  printSeperator();
  return 0;
}

/**
  @brief   Handle "clear" command
  @param   len     --> command length
           paraNum --> number of parameters
  @retval  0 --> success
          -1 --> Command format error
*/
int cmdClear(int len, int paraNum)
{
  if(paraNum != 1){
    return -1;
  }
  if(myVR.clear() == 0){
    printSeperator();
    Serial.println("Recognizer cleared.");
    printSeperator();
  }
  return 0;
}

/**
  @brief   Handle "vr" command
  @param   len     --> command length
           paraNum --> number of parameters
  @retval  0 --> success
          -1 --> Command format error
*/
int cmdVR(int len, int paraNum)
{
  int ret;
  if(paraNum != 1){
    return -1;
  }
  ret = myVR.checkRecognizer(buf);
  if(ret<=0){
    return -1;
  }
  printSeperator();
  printCheckRecognizer(buf);
  printSeperator();
  return 0;
}

/**
  @brief   Handle "record" command
  @param   len     --> command length
           paraNum --> number of parameters
  @retval  0 --> success
          -1 --> Command format error
*/
int cmdRecord(int len, int paraNum)
{
  int ret;
  if(paraNum == 1){
      ret = myVR.checkRecord(buf);
      if(ret>=0){
        printCheckRecord(buf, ret);
      }else{
        Serial.println("Check record failed.");
      }
  }else if(paraNum < 9){
    for(int i=2; i<=paraNum; i++){
      findPara(len, i, &paraAddr);
      records[i-2] = atoi((char *)paraAddr);
      if(records[i-2] == 0 && *paraAddr != '0'){
        return -1;
      }      
    }
    
    ret = myVR.checkRecord(buf, records, paraNum-1);    // auto clean duplicate records
    if(ret>=0){
      printSeperator();
      printCheckRecord(buf, ret);
    }else{
      Serial.println("Check record failed.");
    }
  }else{
    return -1;
  }
  return 0;
}

/**
  @brief   Handle "sigtrain" command
  @param   len     --> command length
           paraNum --> number of parameters
  @retval  0 --> success
          -1 --> Command format error
*/
int cmdSigTrain(int len, int paraNum)
{
  int ret, sig_len;
  uint8_t *lastAddr;
  if(paraNum < 2){
    return -1;
  }
  
  findPara(len, 2, &paraAddr);
  records[0] = atoi((char *)paraAddr);
  if(records[0] == 0 && *paraAddr != '0'){
    return -1;
  }
  
  findPara(len, 3, &paraAddr);
  sig_len = findPara(len, paraNum, &lastAddr);
  sig_len +=( (unsigned int)lastAddr - (unsigned int)paraAddr );
  
  
  ret = myVR.trainWithSignature(records[0], paraAddr, sig_len, buf);
//  ret = myVR.trainWithSignature(records, paraNum-1);
  if(ret >= 0){
    printSigTrain(buf, ret);
  }else{
    Serial.println("Train failed.");
  }
  return 0;
}

/**
  @brief   Handle "getsig" command
  @param   len     --> command length
           paraNum --> number of parameters
  @retval  0 --> success
          -1 --> Command format error
*/
int cmdGetSig(int len, int paraNum)
{
  int ret;
  if(paraNum != 2){
    return -1;
  }
  
  findPara(len, 2, &paraAddr);
  records[0] = atoi((char *)paraAddr);
  if(records[0] == 0 && *paraAddr != '0'){
    return -1;
  }
  
  ret = myVR.checkSignature(records[0], buf);
  if(ret == 0){
    Serial.println("Signature isn't set.");
  }else if(ret > 0){
    Serial.print("Signature:");
    printSignature(buf, ret);
    Serial.println();
  }else{
    Serial.println("Get sig error.");
  }
  return 0;
}

/**
  @brief   Handle "test" command
  @param   len     --> command length
           paraNum --> number of parameters
  @retval  0 --> success
          -1 --> Command format error
*/
int cmdTest(int len, int paraNum)
{
  Serial.println("TEST is not supported.");
  return 0;
}


/*****************************************************************************/
/**
  @brief   Print signature, if the character is invisible, 
           print hexible value instead.
  @param   buf     --> command length
           len     --> number of parameters
*/
void printSignature(uint8_t *buf, int len)
{
  int i;
  for(i=0; i<len; i++){
    if(buf[i]>0x19 && buf[i]<0x7F){
      Serial.write(buf[i]);
    }
    else{
      Serial.print("[");
      Serial.print(buf[i], HEX);
      Serial.print("]");
    }
  }
}

/**
  @brief   Print signature, if the character is invisible, 
           print hexible value instead.
  @param   buf  -->  VR module return value when voice is recognized.
             buf[0]  -->  Group mode(FF: None Group, 0x8n: User, 0x0n:System
             buf[1]  -->  number of record which is recognized. 
             buf[2]  -->  Recognizer index(position) value of the recognized record.
             buf[3]  -->  Signature length
             buf[4]~buf[n] --> Signature
*/
void printVR(uint8_t *buf)
{
  Serial.println("VR Index\tGroup\tRecordNum\tSignature");

  Serial.print(buf[2], DEC);
  Serial.print("\t\t");

  if(buf[0] == 0xFF){
    Serial.print("NONE");
  }
  else if(buf[0]&0x80){
    Serial.print("UG ");
    Serial.print(buf[0]&(~0x80), DEC);
  }
  else{
    Serial.print("SG ");
    Serial.print(buf[0], DEC);
  }
  Serial.print("\t");

  Serial.print(buf[1], DEC);
  Serial.print("\t\t");
  if(buf[3]>0){
    printSignature(buf+4, buf[3]);
  }
  else{
    Serial.print("NONE");
  }
  Serial.println("\r\n");
}

/**
  @brief   Print seperator. Print 80 '-'.
*/
void printSeperator()
{
  for(int i=0; i<80; i++){
    Serial.write('-');
  }
  Serial.println();
}

/**
  @brief   Print recoginizer status.
  @param   buf  -->  VR module return value when voice is recognized.
             buf[0]     -->  Number of valid voice records in recognizer
             buf[i+1]   -->  Record number.(0xFF: Not loaded(Nongroup mode), or not set (Group mode)) (i= 0, 1, ... 6)
             buf[8]     -->  Number of all voice records in recognizer
             buf[9]     -->  Valid records position indicate.
             buf[10]    -->  Group mode indicate(FF: None Group, 0x8n: User, 0x0n:System
*/
void printCheckRecognizer(uint8_t *buf)
{
  Serial.print("All voice records in recognizer: ");
  Serial.println(buf[8], DEC);
  Serial.print("Valid voice records in recognizer: ");
  Serial.println(buf[0], DEC);
  if(buf[10] == 0xFF){
    Serial.println("VR is not in group mode.");
  }
  else if(buf[10]&0x80){
    Serial.print("VR is in user group mode:");
    Serial.println(buf[10]&0x7F, DEC);
  }
  else{
    Serial.print("VR is in system group mode:");
    Serial.println(buf[10], DEC);
  }
  Serial.println("VR Index\tRecord\t\tComment");
  for(int i=0; i<7; i++){
    Serial.print(i, DEC);
    Serial.print("\t\t");
    if(buf[i+1] == 0xFF){
      if(buf[10] == 0xFF){
        Serial.print("Unloaded\tNONE");
      }else{
        Serial.print("Not Set\t\tNONE");
      }
    }
    else{
      Serial.print(buf[i+1], DEC);
      Serial.print("\t\t");
      if(buf[9]&(1<<i)){
        Serial.print("Valid");
      }
      else{
        Serial.print("Untrained");
      }
    }

    Serial.println();
  } 
}

/**
  @brief   Print record train status.
  @param   buf  -->  Check record command return value
             buf[0]     -->  Number of checked records
             buf[2i+1]  -->  Record number.
             buf[2i+2]  -->  Record train status. (00: untrained, 01: trained, FF: record value out of range)
             (i = 0 ~ buf[0]-1 )
           num  -->  Number of trained records
*/
void printCheckRecord(uint8_t *buf, int num)
{
  Serial.print("Check ");
  Serial.print(buf[0], DEC);
  Serial.println(" records.");

  Serial.print(num, DEC);
  if(num>1){
    Serial.println(" records trained.");
  }
  else{
    Serial.println(" record trained.");
  }

  for(int i=0; i<buf[0]*2; i += 2){
    Serial.print(buf[i+1], DEC);
    Serial.print("\t-->\t");
    switch(buf[i+2]){
      case 0x01:
        Serial.print("Trained");
        break;
      case 0x00:
        Serial.print("Untrained");
        break;
      case 0xFF:
        Serial.print("Record value out of range");
        break;
      default:
        Serial.print("Unknown Stauts");
        break;
    }
    Serial.println();
  }
}

/**
  @brief   Print check user group result.
  @param   buf  -->  Check record command return value
             buf[8i]    -->  group number.
             buf[8i+1]  -->  group position 0 status.
             buf[8i+2]  -->  group position 1 status.
               ...                ...
             buf[8i+6]  -->  group position 5 status.
             buf[8i+7]  -->  group position 6 status.
             (i = 0 ~ len)
           len  -->  number of checked groups
*/
void printUserGroup(uint8_t *buf, int len)
{
  int i, j;
  Serial.println("Check User Group:");
  for(i=0; i<len; i++){
    Serial.print("Group:");
    Serial.println(buf[8*i]);
    for(j=0; j<7; j++){
      if(buf[8*i+1+j] == 0xFF){
        Serial.print("NONE\t");
      }else{
        Serial.print(buf[8*i+1+j], DEC);
        Serial.print("\t");
      }
    }
    Serial.println();
  }
}

/**
  @brief   Print "load" command return value.
  @param   buf  -->  "load" command command return value
             buf[0]    -->  number of records which are load successfully.
             buf[2i+1]  -->  record number
             buf[2i+2]  -->  record load status.
                00 --> Loaded 
                FC --> Record already in recognizer
                FD --> Recognizer full
                FE --> Record untrained
                FF --> Value out of range"
             (i = 0 ~ len-1 )
           len  -->  length of buf
*/
void printLoad(uint8_t *buf, uint8_t len)
{
  if(len == 0){
    Serial.println("Load Successfully.");
    return;
  }else{
    Serial.print("Load success: ");
    Serial.println(buf[0], DEC);
  }
  for(int i=0; i<len-1; i += 2){
    Serial.print("Record ");
    Serial.print(buf[i+1], DEC);
    Serial.print("\t");
    switch(buf[i+2]){
      case 0:
        Serial.println("Loaded");
        break;
      case 0xFC:
        Serial.println("Record already in recognizer");
        break;
      case 0xFD:
        Serial.println("Recognizer full");
        break;
      case 0xFE:
        Serial.println("Record untrained");
        break;
      case 0xFF:
        Serial.println("Value out of range");
        break;
      default:
        Serial.println("Unknown status");
        break;
    }
  }
}

/**
  @brief   Print "train" command return value.
  @param   buf  -->  "load" command command return value
             buf[0]    -->  number of records which are trained successfully.
             buf[2i+1]  -->  record number
             buf[2i+2]  -->  record load status.
                00 --> Trained 
                FE --> Train Time Out
                FF --> Value out of range"
             (i = 0 ~ len-1 )
           len  -->  length of buf
*/
void printTrain(uint8_t *buf, uint8_t len)
{
  if(len == 0){
    Serial.println("Train Finish.");
    return;
  }else{
    Serial.print("Train success: ");
    Serial.println(buf[0], DEC);
  }
  for(int i=0; i<len-1; i += 2){
    Serial.print("Record ");
    Serial.print(buf[i+1], DEC);
    Serial.print("\t");
    switch(buf[i+2]){
      case 0:
        Serial.println("Trained");
        break;
      case 0xFE:
        Serial.println("Train Time Out");
        break;
      case 0xFF:
        Serial.println("Value out of range");
        break;
      default:
        Serial.print("Unknown status ");
        Serial.println(buf[i+2], HEX);
        break;
    }
  }
}

/**
  @brief   Print "train" command return value.
  @param   buf  -->  "load" command command return value
             buf[0]  -->  number of records which are trained successfully.
             buf[1]  -->  record number
             buf[2]  -->  record load status.
                00 --> Trained 
                F0 --> Trained, signature truncate
                FE --> Train Time Out
                FF --> Value out of range"
             buf[3] ~ buf[len-1] --> Signature.
           len  -->  length of buf
*/
void printSigTrain(uint8_t *buf, uint8_t len)
{
  if(len == 0){
    Serial.println("Train With Signature Finish.");
    return;
  }else{
    Serial.print("Success: ");
    Serial.println(buf[0], DEC);
  }
  Serial.print("Record ");
  Serial.print(buf[1], DEC);
  Serial.print("\t");
  switch(buf[2]){
    case 0:
      Serial.println("Trained");
      break;
    case 0xF0:
      Serial.println("Trained, signature truncate");
      break;
    case 0xFE:
      Serial.println("Train Time Out");
      break;
    case 0xFF:
      Serial.println("Value out of range");
      break;
    default:
      Serial.print("Unknown status ");
      Serial.println(buf[2], HEX);
      break;
  }
  Serial.print("SIG: ");
  Serial.write(buf+3, len-3);
  Serial.println();
}
