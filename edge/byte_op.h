#ifndef __BYTE_OP_H__ /* __BYTE_OP_H__ 가 아직 정의되지 않았다면 */
#define __BYTE_OP_H__ /*__BYTE_OP_H__ 를 정의하여 이 헤더 파일이 여러 번 include되는 것을 방지 */

#include <cstdio> /* printf 사용하기 위해 include */
#include <cstring> /* memset 사용하기 위해 include */

/* Variable <-> Memory conversion (Big Endian)
  데이터를 Big Endian 형식으로 메모리에 저장하거나,
  메모리 데이터를 변수로 복원하기 위한 매크로 
  
  네트워크 표준은 Big Endian 형식을 사용하므로, variable을 Big Endian 형식으로 저장하거나 복원하는 코드
  Intel CPU 같이 프로세서가 Little Endian을 채택했더라도 네트워크 통신에서는 Big Endian 형식을 사용하게함 */


#define VAR_TO_MEM_1BYTE_BIG_ENDIAN(v, p) \
  *(p++) = v & 0xff;
/* 1 byte variable >>> Memory (Big Endian)
  변수 v의 하위 1 byte를 현재 p가 가리키는 메모리에 저장한 뒤, 포인터 p를 다음 위치로 이동
  이때 v와 0xff를 AND 연산하여 하위 1 byte만 추출 후 p에 저장 */


#define VAR_TO_MEM_2BYTES_BIG_ENDIAN(v, p) \
  *(p++) = (v >> 8) & 0xff; *(p++) = v & 0xff;
/* 2 bytes variable >>> Memory (Big Endian)
  MSB부터 저장하기 위해, v를 8bit 만큼 shift 하여 변수 v의 상위 byte를 먼저 메모리 p에 저장
  이후 포인터 p를 다음 위치로 이동 하고
  나머지 1 byte를 저장 후 포인터 p를 다음 위치로 이동 */
  

#define VAR_TO_MEM_4BYTES_BIG_ENDIAN(v, p) \
  *(p++) = (v >> 24) & 0xff; *(p++) = (v >> 16) & 0xff; *(p++) = (v >> 8) & 0xff; *(p++) = v & 0xff;
/* 4 bytes variable >>> Memory (Big Endian)
  MSB부터 저장하기 위해, v를 각각 24bit, 16bit, 8bit 만큼 shift, 변수 v의 상위 byte들을 순서대로 메모리 p에 저장하고 p를 다음 위치로 이동시키고를 반복
  나머지 1 byte를 저장 후 포인터 p를 다음 위치로 이동 */


#define MEM_TO_VAR_1BYTE_BIG_ENDIAN(p, v) \
  v = (p[0] & 0xff); p += 1;
/* Memory >>> 1 byte variable
  메모리 p의 1 byte 데이터를 변수 v에 저장하고, 포인터 p를 다음 위치로 이동 */


#define MEM_TO_VAR_2BYTES_BIG_ENDIAN(p, v) \
  v = ((p[0] & 0xff) << 8) | (p[1] & 0xff); p += 2;
/* Memory >>> 2 bytes variable (Big Endian)
  p[0]을 상위 1 byte 위치로 이동시키기 위해 8bit shift 후
  p[1]과 OR 연산으로 결합하여 v 생성
  이후 포인터 p를 2 bytes 이동 */


#define MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, v) \
  v = ((p[0] & 0xff) << 24) | ((p[1] & 0xff) << 16) | ((p[2] & 0xff) << 8) | (p[3] & 0xff); p += 4;
/* Memory >>> 4 bytes variable (Big Endian)
  p[0]~p[3]을 각각 24bit, 16bit, 8bit, 0bit 만큼 shift 후 OR 연산으로 결합하여 v 생성
  이후 포인터 p를 4 bytes 이동 */


#define PRINT_MEM(p, len) \
  printf("Print buffer:\n  >> "); \
  for (int i=0; i<len; i++) { \
    printf("%02x ", p[i]); \
    if (i % 16 == 15) printf("\n  >> "); \
  } \
  printf("\n");
/* 메모리 버퍼를 출력하는 매크로
  버퍼를 16진수 형식으로 출력 
  %02x는 출력 시 2자리 16진수로 출력하라는 의미
  16 bytes 마다 줄 바꿈*/

  
#endif /* __BYTE_OP_H__ 조건문 끝 */
