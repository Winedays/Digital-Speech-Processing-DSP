#include <iostream>
#include <fstream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Ngram.h"
#include "Vocab.h"
using namespace std;


int read_testdata(char *file, char *data[]);
int read_map(char *file, char *data[]);
double getbigramprob(const char *w1, const char *w2, Vocab &voc, Ngram &lm);

static FILE *open_or_die(const char *filename, const char *ht)
{
	FILE *fp = fopen(filename, ht);
	if (fp == NULL) {
		perror(filename);
		exit(1);
	}
	return fp;
}

//argc是argument count(參數個數)的縮寫，代表包括指令本身的參數個數。系統會自動計算所輸入的參數個數。
int main(int argc, char *argv[])
{
	char* testdata_file;
	char* map_file;
	char* lm_file;
	int order;

	// init. file path
	if (argc == 1) {
		// if no argument variable , init. file path
		order = 2;

		char* model_name = "testdata/1.txt";
		testdata_file = new char[strlen(model_name)];
		strcpy(testdata_file, model_name);

		model_name = "ZhuYin-Big5.map";
		map_file = new char[strlen(model_name)];
		strcpy(map_file, model_name);

		model_name = "bigram.lm";
		lm_file = new char[strlen(model_name)];
		strcpy(lm_file, model_name);

	}
	else {
		testdata_file = argv[1];
		map_file = argv[2];
		lm_file = argv[3];
		order = atoi(argv[4]); // n-gram : it will 2(bi-gram) if have not set
	}

	// read file
	char *test_data[51] , *map[15000];
	int data_count = read_testdata(testdata_file, test_data);
	int map_count = read_map(map_file, map);
	Vocab voc;
	Ngram lm(voc, order);
	{
		File lmFile(lm_file, "r"); // language model 
		lm.read(lmFile);
		lmFile.close();
	}

	VocabIndex G_viterbi_map[10000][1000];
	int G_viterbi_path[10000][1000];
	double G_viterbi_prob[8700][1000];
	
	//run Viterbi
	for (int i = 0; i < data_count; i++)   // 每個句子
	{
		int word_count = strlen( test_data[i] ) / 2;
		//printf("%s\n", test_data[i]);
		//printf("%d\n", word_count);
		for (int j = 0; j < word_count; j++)  // 每個字
		{
			char word[3], pre_word[3];
			strncpy(word, test_data[i] + j * 2, 2); word[2] = '\0'; // word: 該注音的第i個國字
			if (j != 0) {
				strncpy(pre_word, test_data[i] + (j - 1) * 2, 2); pre_word[2] = '\0';
				//printf("pre_word %s : ", pre_word);
			}

			// find mapping
			int map_index = -1;
			for (int k = 0; k < map_count; k++) {
				char map_word[3];
				strncpy(map_word, map[k], 2); map_word[2] = '\0';
				if (strcmp(map_word, word) == 0) {
					map_index = k;
				}
			}

			int wordmap_size = strlen(map[map_index]) / 2;
			for (int k = 1; k < wordmap_size; k++)
			{
				char map_word[3];
				strncpy(map_word, map[map_index] + k * 2, 2); map_word[2] = '\0'; // word: 該注音的第i個mapping字
				G_viterbi_map[k][j] = voc.getIndex(map_word);
			}

			double max_prob = -1000.0 , pre_prob = 0;
			int prob_index = 0;
			char prob_word[3] ;
			// cal prob.
			for (int k = 1; k < wordmap_size; k++)
			{
				char map_word[3] ;
				strncpy(map_word, map[map_index] + k * 2, 2); map_word[2] = '\0'; // word: 該注音的第i個mapping字

				// 第一個字
				if (j == 0)
				{
					int prob = getbigramprob("<unk>", map_word, voc, lm);
					
					G_viterbi_prob[k][j] = getbigramprob("<unk>", map_word, voc, lm);
					G_viterbi_path[k][j] = -1 ;
				}
				else
				{
					for (int m = 1; m < wordmap_size; m++)
					{
						int prob = 0 ;
						if (G_viterbi_map[m][j-1] == -1) {
							prob = getbigramprob("<unk>", map_word, voc, lm);
						}
						else {
							prob = getbigramprob(voc.getWord(G_viterbi_map[m][j - 1]), map_word, voc, lm) + G_viterbi_prob[m][j-1];
						}
						if (max_prob < prob) {
							max_prob = prob;
							pre_prob = prob;
							prob_index = k;
							strncpy(prob_word, voc.getWord(prob_index), 2); prob_word[2] = '\0';
						}
					}
					G_viterbi_prob[k][j] = max_prob;
					G_viterbi_path[k][j] = prob_index;
				}
			}
			printf("%s -> %s , %d ", word, prob_word, prob_index);

		}

		printf("\n");
	}
	//system("pause");
}

// read testdata file 
int read_testdata(char *file, char *data[])
{
	FILE *fp = open_or_die(file, "r");

	char words[250];
	for (int i = 0; i < 51; i++) {
		data[i] = (char *)malloc(sizeof(char) * 250);
		strcpy(data[i], "\0");
	}

	int seq_count = 0;
	// 若想遇到空格繼續讀取，讀取完整的一行數據，則用如下語句：
	while (fscanf(fp, "%[^\n]%*c", words) > 0)
	{
		if (words[0] == '\0' || words[0] == '\n') continue;

		char * pch;
		pch = strtok(words, " ");
		while (pch != NULL)
		{
			strcat(data[seq_count], pch);
			pch = strtok(NULL, " ");
		}
		//strcpy(data[seq_count], words);
		//printf("%s\n", data[seq_count]);
		seq_count++;
	}
	//fclose(fp);
	return seq_count;
}

// read testdata file 
int read_map(char *file, char *data[])
{
	FILE *fp = open_or_die(file, "r");

	char mapdata[5000];
	for (int i = 0; i < 15000; i++) {
		data[i] = (char *)malloc(sizeof(char) * 5000);
		strcpy(data[i], "\0");
	}

	int seq_count = 0;
	// 若想遇到空格繼續讀取，讀取完整的一行數據，則用如下語句：
	while (fscanf(fp, "%[^\n]%*c", mapdata) > 0) // && seq_count < 35
	{
		if (mapdata[0] == '\0' || mapdata[0] == '\n') continue;
		char * pch;
		pch = strtok(mapdata, "\t");
		strcat(data[seq_count], pch);
		pch = strtok(NULL, " ");
		while (pch != NULL)
		{
			strcat(data[seq_count], pch);
			pch = strtok(NULL, " ");
		}
		//printf("%s\n", data[seq_count]);
		seq_count++;
	}
	//fclose(fp);
	return seq_count;
}

// get p(w2 | w1) -- bigram
double getbigramprob(const char *w1, const char *w2, Vocab &voc, Ngram &lm)
{
	VocabIndex wid1 = voc.getIndex(w1);
	VocabIndex wid2 = voc.getIndex(w2);

	if (wid1 == Vocab_None)  //oov
		wid1 = voc.getIndex(Vocab_Unknown);
	if (wid2 == Vocab_None)  //oov
		wid2 = voc.getIndex(Vocab_Unknown);

	VocabIndex context[] = { wid1, Vocab_None };
	return lm.wordProb(wid2, context);
}