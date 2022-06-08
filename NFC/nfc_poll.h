#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

#define MAX_DEVICE 10

static nfc_device *pnd = NULL;
static nfc_context *context;

uint8_t nfc_id [MAX_DEVICE][4] = {
	{0xc1, 0x35, 0x42, 0x8f},
	{0x3a, 0x31, 0x10, 0x21},
	{0x4c, 0x0b, 0x20, 0x49},
	{0x61, 0xad, 0xa8, 0x56}, 
};

int floor_id [MAX_DEVICE]={
	2,
	3,
	2,
	3,
};

static void print_hex(const uint8_t *pbtData, const size_t size_byte) {
	size_t size_i;
	
	for(size_i = 0; size_i < size_byte; size_i++)
		printf("%02x ", pbtData[size_i]);
	
	printf("\n");
}

int cmp_hex(const uint8_t *pollData) {
	int size = sizeof(uint8_t);
	
	for(int i=0;i<MAX_DEVICE;i++) {
		size_t j;
		for(j = 0;j < 4; j++) {
			if(pollData[j] != nfc_id[i][j]) break;
		}
		if(j == 4) return floor_id[i];
	}
	return -1;
}


int nfc_poll()
{
	bool verbose = false;
	uint8_t *read_UID;

	const nfc_modulation nmModulations[5] = {
    		{ .nmt = NMT_ISO14443A, .nbr = NBR_106 },
   		{ .nmt = NMT_ISO14443B, .nbr = NBR_106 },
   		{ .nmt = NMT_FELICA, .nbr = NBR_212 },
    		{ .nmt = NMT_FELICA, .nbr = NBR_424 },
   		{ .nmt = NMT_JEWEL, .nbr = NBR_106 },
  	};

	nfc_target nt;
	int result = 0;
	nfc_init(&context);
	
	if (context == NULL) {
    		//ERR("Unable to init libnfc (malloc)");
    		exit(EXIT_FAILURE);
  	}

  	pnd = nfc_open(context, NULL);

 	 if (pnd == NULL) {
 	   //ERR("%s", "Unable to open NFC device.");
 	   nfc_exit(context);
 	   exit(EXIT_FAILURE);
 	 }

 	 if (nfc_initiator_init(pnd) < 0) {
   		 nfc_perror(pnd, "nfc_initiator_init");
  		 nfc_close(pnd);
  		 nfc_exit(context);
  		 exit(EXIT_FAILURE);
  	}

	 printf("NFC is open\n");
	
	 result = nfc_initiator_select_passive_target(pnd, nmModulations[0], NULL, 0, &nt);	//NMT_ISO14443A 장치만 사용(휴대폰, 키 다됨)

	if (result > 0)
	{
		//print_nfc_target(&nt, verbose);//nfc로부터 장치를 읽고 결과를 출력한 뒤 nfc 종료(내부에서 str_nfc_target으로 값을 읽어오는 구조)
		//str_nfc_target(&s, &nt, verbose);//nfc로부터 장치를 읽고 char*s에 값을 저장해주는 함수(내부구조 카톡참조)
		read_UID = nt.nti.nai.abtUid;
		printf("UID(NFC ID %c) : ", (read_UID[0] == 0x08 ? '3' : '1'));
		print_hex(read_UID, nt.nti.nai.szUidLen);//UidLen을 받아오는 이유는 지원기기(nmModulations)마다 길이가 다르기 때문. 해당내용이 있는 주소는 카톡 참조
	}
	else
		printf("No target found.\n");

	printf("Waiting for card removing...\n");
	while (0 == nfc_initiator_target_is_present(pnd, NULL)) {}
	nfc_perror(pnd, "fnc_initiator_target_is_present");
	printf("done.\n");

	int floor = cmp_hex(read_UID);
	printf("%d floor\n",floor);

	nfc_close(pnd);
	nfc_exit(context);
	return floor;
}
