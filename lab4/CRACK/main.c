#include "../func.h"

int main (int argc, char *argv[]) {

  int verbose = 0;
  int problem = 0;

  opterr = 0;

	const char *short_options = "v";
	const struct option long_options[] = {
		{"verbose", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	int rez;
	int option_index;

	while ((rez=getopt_long(argc,argv,short_options,
		long_options,&option_index))!=-1){

		switch (rez){
			case 'v': {
				verbose = 1;
				break;
			};
			case '?': default: {
        problem = 1;
				printf("Found unknown option\n");
				break;
			};
		};
	};

  if (problem)
    return 0;

  char *filename = argv[argc-1];
  FILE *in;
  int pass = 0;

  if ((in = fopen(filename, "r")) == NULL) {
		printf("Please, enter correct filename\n");
    return 0;
  }

  pass = checker(in);

  fclose(in);

  if (!(pass)) {
    printf("Invalid file.\n");
    return 0;
  }

  printf("Valid file!\n");

  int hash_type = 0;
  int ci_type = 0;
  int KEY_LEN[NUM_TYPES_CIPHERS] = {KEY_LEN_3DES, KEY_LEN_AES128, KEY_LEN_AES192, KEY_LEN_AES256};
  int IV_LEN[NUM_TYPES_CIPHERS] = {IV_LEN_3DES, IV_LEN_AES128, IV_LEN_AES192, IV_LEN_AES256};

  char nonce[NONCE_LEN];
  char iv[MAX_IV_LEN];
  char ciphertext[MAX_TEXT_LEN];
  int ct_len = 0;
  unsigned int_pwrd;

  in = fopen(filename, "r");

  readinfo(in, &hash_type, &ci_type, nonce, iv, ciphertext, &ct_len);

  print_data(hash_type, ci_type, nonce, iv, ciphertext, ct_len);

  fclose(in);

  printf("\nStart cracking\n\n");

  char password[PWRD_LEN];
  char key[KEY_LEN[ci_type]];
  char opentext[ct_len];
  int isright = 0;

  char *tmp_key;
  tmp_key = (char *) malloc(KEY_LEN[ci_type] * sizeof(char));

  double time_in_seconds = 0;

  clock_t start = clock();
  clock_t current = clock();
  clock_t previous = clock();

  printf("Current: 00000000 - 00ffffff\n");

  for (unsigned i = 0; i <= UINT_MAX; i++) {
    int_pwrd = i;
    isright = 1;

    for (int j = 0; j < PWRD_LEN; j++) {
      password[PWRD_LEN - 1 - j] = (char) int_pwrd % LEN_CHAR;
      int_pwrd /= LEN_CHAR;
    }


    if ((!(i & 0xffffff)) && (verbose) && (i != 0)) {
      previous = current;
      current = clock();

      printf("Current: %08x - %08x |", i, (i + 0xffffff));

      time_in_seconds = (double) (current - previous) * 1000.0 / CLOCKS_PER_SEC;
      printf("Current speed: %6.0f c/s | ", (0x1000000 / time_in_seconds));

      time_in_seconds = (double) (current - start) * 1000.0 / CLOCKS_PER_SEC;
      printf("Average speed: %6.0f c/s\n", (i / time_in_seconds));
    }



    if (hash_type == 0) {

      char hmac[HMAC_MD5_LEN];

      hmac_md5((unsigned char *) nonce, NONCE_LEN, (unsigned char *) password, PWRD_LEN, (unsigned char *) hmac);

      for(int i = 0; i < HMAC_MD5_LEN; i++)
        key[i] = hmac[i];

      if (HMAC_MD5_LEN < KEY_LEN[ci_type]) {

        int delta = KEY_LEN[ci_type] - HMAC_MD5_LEN;
        char tmp_hmac[HMAC_MD5_LEN];
        hmac_md5((unsigned char *) hmac, HMAC_MD5_LEN, (unsigned char *) password, PWRD_LEN, (unsigned char *) tmp_hmac);

        for (int i = 0; i < delta; i++)
          key[KEY_LEN[ci_type] - delta + i] = tmp_hmac[i];

      }
    } else {

      char hmac[HMAC_SHA1_LEN];

      hmac_sha1((unsigned char *) nonce, NONCE_LEN, (unsigned char *) password, PWRD_LEN, (unsigned char *) hmac);

      if (HMAC_SHA1_LEN > KEY_LEN[ci_type]) {

        for(int i = 0; i < KEY_LEN[ci_type]; i++)
          key[i] = hmac[i];

      } else if (HMAC_SHA1_LEN < KEY_LEN[ci_type]) {

        for(int i = 0; i < HMAC_SHA1_LEN; i++)
          key[i] = hmac[i];

        int delta = KEY_LEN[ci_type] - HMAC_SHA1_LEN;
        char tmp_hmac[HMAC_SHA1_LEN];
        hmac_md5((unsigned char *) hmac, HMAC_SHA1_LEN, (unsigned char *) password, PWRD_LEN, (unsigned char *) tmp_hmac);

        for (int i = 0; i < delta; i++)
          key[KEY_LEN[ci_type] - delta + i] = tmp_hmac[i];

      }
    }



    if (ci_type == 0) {
      des3_cbc_decrypt((unsigned char *) ciphertext, ct_len, (unsigned char *)iv, (unsigned char *)key, (unsigned char *)opentext);
    } else {
      aes_cbc_decrypt((unsigned char *) ciphertext, ct_len, (unsigned char *)iv, (unsigned char *)key, (unsigned char *)opentext, IV_LEN[ci_type] * BYTE_LEN);
    }


    for (int j = 0; j < NULL_CHECK_LEN; j++) {
      if (opentext[j] != 0){
        isright = 0;
        break;
      }
    }

    if (i == UINT_MAX)
      break;

    if (isright)
      break;

  }

  current = clock();

  free(tmp_key);

  printf("Found: ");
  for (int i = 0; i < PWRD_LEN; i++) {
    printf("%02hhx", password[i]);
  }

  if (verbose) {
    time_in_seconds = (double) (current - start) * 1000.0 / CLOCKS_PER_SEC;
    printf(" | Average speed: %6.0f c/s\n", (0xffffffff / time_in_seconds));
  }


  printf("\nMessage's text is: \n\n");
  for (int i = NULL_CHECK_LEN; i < ct_len; i++) {
    printf("%c", opentext[i]);
  }

  printf("\n\n");

  return 0;
}
