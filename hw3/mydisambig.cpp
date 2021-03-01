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

char G_zy_table[37][2];
char G_zy_ch_map[37][8700*3];
int G_ch_num[87];
VocabIndex G_viterbi_map[8700][87];
double G_viterbi_prob[8700][87];
int G_viterbi_path[8700][87];
char G_viterbi_out_ch[87][3];

int read_testdata(char *file, char *data[]);
int read_map(char *file, char *data[]);
double getbigramprob(const char *w1, const char *w2, Vocab &voc, Ngram &lm);
int is_zy(char *pch);
void get_G_ch_num(int zy_idx, int cnt) ;
void get_G_viterbi_map(int zy_idx, int cnt, char *pch, Vocab &voc) ;
void get_G_viterbi_prob_and_G_viterbi_path(int zy_idx, int cnt, char *pch, char *word, Vocab &voc, Ngram &lm);

static FILE *open_or_die(const char *filename, const char *ht)
{
	FILE *fp = fopen(filename, ht);
	if (fp == NULL) {
		perror(filename);
		exit(1);
	}
	return fp;
}

// 計算  G_viterbi_out_ch [cnt] [0~2]
void get_G_viterbi_out_ch( int cnt, char *word)
{
	strncpy(G_viterbi_out_ch[cnt], word, 2);
}

// 輸出結果
void print_out(int cnt, char *text, Vocab &voc)
{
	if(cnt == 0)
	{
		cout << "<s>" << text << "</s>" << endl;
	}
	else
	{
		double out_max = -87000;
		int out_max_idx = -1;
		VocabIndex output[87];

		for(int i = 0 ; i < G_ch_num[cnt-1]; i++)
		{
			if(G_viterbi_prob[i][cnt-1] > out_max)
			{
				out_max = G_viterbi_prob[i][cnt-1];
				out_max_idx = i;
			}
		}

		for (int i = 0; i < cnt; i++)
		{
			output[cnt - 1 - i] = G_viterbi_map[out_max_idx][cnt - 1 - i];
			out_max_idx = G_viterbi_path[out_max_idx][cnt - 1 - i];
		}

		printf("<s> ");
		for(int i = 0; i < cnt; i++)
		{
			cout << ( (output[i] != -1) ? ( voc.getWord(output[i]) ) : ( G_viterbi_out_ch[i] ) ) << " ";
		}
		printf("</s>\n");
	}
}

int main( int argc, char * argv[] )
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

	fstream test_data, map_data, zy_table_data;
	test_data.open(testdata_file,ios::in ); // 測資 
	map_data.open(map_file,ios::in ); // 注音-國字 map
	zy_table_data.open( "zy_table.txt" ,ios::in ); // 注音表

	// read file
	char *testdata[51], *map[15000];
	int data_count = read_testdata(testdata_file, testdata);
	int map_count = read_map(map_file, map);

	Vocab voc;
 	Ngram lm(voc, order);
	{
		File lmFile(lm_file, "r"); // language model 
		lm.read(lmFile);
		lmFile.close();
	}

	char text[870]; // 裝整行的字 
	char word[2]; // 裝一個字 
	char *pch; // 切字時用來裝字
	int cnt; // 讀到測資的第幾個字了
	int zy_idx; // 該字在注音表中的index (國字則為-1)

	// 把 37 個注音填到 G_zy_table[37][2]
	for(int i = 0 ; i < 37; i++)
	{
		zy_table_data >> G_zy_table[i];
	}

	// read file
	data_count = read_testdata(testdata_file, testdata);
	map_count = read_map(map_file, map);

	// 填表 G_zy_ch_map[i] [8700], 例如: G_zy_ch_map[2] = ㄇ開頭的所有國字
	while(map_data >> word)
	{
		int i;
		zy_idx = -1;
		for(i = 0 ; i < 37; i++)
		{
			if(strncmp(word, G_zy_table[i], 2) == 0)
			{
				zy_idx = i; 
				break;
			}
		}
		(zy_idx == -1) ? ( map_data.getline(text, 869) ) : ( map_data.getline(G_zy_ch_map[i], (8700*3-1) ) );
	}

	// read file
	data_count = read_testdata(testdata_file, testdata);
	map_count = read_map(map_file, map);

	while(test_data.getline(text, 869))
	{
		cnt = 0;

		// 迭代測資某行的每個字, 套用演算法
		pch = strtok(text, " ");
		while(pch != NULL)
		{
			// 檢查這個字是不是注音, 是則回傳該注音在注音表中的index, 否則回傳-1
			zy_idx = is_zy(pch);

			// 計算 G_ch_num [cnt]
			get_G_ch_num(zy_idx, cnt);

			// 計算 G_viterbi_map [i][cnt] 
			get_G_viterbi_map(zy_idx, cnt, pch, voc);

			// 計算 G_viterbi_prob[i][cnt], G_viterbi_path[i][cnt]
			get_G_viterbi_prob_and_G_viterbi_path(zy_idx, cnt, pch, word, voc, lm);

			// 算 G_viterbi_out_ch[cnt][0~2] //先不要管這個
			get_G_viterbi_out_ch(cnt, word);

			cnt++;

			pch = strtok(NULL, " ");
		}

		// 輸出該行結果 
		print_out(cnt, text, voc);
	}



	test_data.close();
	map_data.close();
	zy_table_data.close();

	return 0;
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


// 檢查這個字是不是注音, 是則回傳該注音在注音表中的 index, 否則回傳-1
int is_zy(char *pch)
{
	int zy_idx = -1;
	for (int i = 0; i < 37; i++)
	{
		if (strncmp(pch, G_zy_table[i], 2) == 0)
		{
			zy_idx = i;
			break;
		}
	}
	return zy_idx;
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

// 計算 G_ch_num [cnt]
// 若是國字, 就是1
// 若是注音, 就是該注音有幾個 map 到的國字數
void get_G_ch_num(int zy_idx, int cnt)
{
	G_ch_num[cnt] = (zy_idx == -1) ? 1 : (strlen(G_zy_ch_map[zy_idx]) / 3);
}


// 計算 G_viterbi_map [i][cnt]
// 若是國字, G_viterbi_map[0][cnt] = getIndex (該字)
// 若是注音, G_viterbi_map[i][cnt] = getIndex (那個注音 map 到的第 i 個國字)
void get_G_viterbi_map(int zy_idx, int cnt, char *pch, Vocab &voc)
{
	char word[2];
	char mapword[3];
	int cpy = cnt ;
	if (zy_idx == -1)
	{
		G_viterbi_map[0][cnt] = voc.getIndex(pch);
		//printf("%s  ", pch);
	}
	else
	{
		for (int i = 0; i < strlen(G_zy_ch_map[zy_idx]) / 3; i++)
		{
			strncpy(word, G_zy_ch_map[zy_idx] + 3 * i + 1 , 2); // word 是該注音的第i個國字 
			strcpy(mapword, word ); // word 是該注音的第i個國字 
			mapword[2] = '\0' ;
			G_viterbi_map[i][cnt] = voc.getIndex(mapword);
			//printf("%s  ", mapword);
		}
	}
}

// 計算 G_viterbi_prob[i][cnt] 與 G_viterbi_path[i][cnt]   ( i 迭代該注音的每個 map 的國字 的index )
void get_G_viterbi_prob_and_G_viterbi_path(int zy_idx, int cnt, char *pch, char *word, Vocab &voc, Ngram &lm)
{
	double prob;
	// 若這個字是注音
	if (zy_idx >= 0)
	{
		for (int i = 0; i < strlen(G_zy_ch_map[zy_idx]) / 3; i++)
		{
			strncpy(word, G_zy_ch_map[zy_idx] + 3 * i + 1, 2); // word: 該注音的第i個國字 

															   // 若在測資該行的第一個字(cnt == 0):
			if (cnt == 0)
			{
				G_viterbi_prob[i][cnt] = getbigramprob("<unk>", word, voc, lm);
				G_viterbi_path[i][cnt] = -1;
			}
			else
			{
				double max_prob = -870000;
				int max_prob_idx = -1;
				for (int j = 0; j < G_ch_num[cnt - 1]; j++)
				{
					prob = (G_viterbi_map[j][cnt - 1] == -1)
						? (getbigramprob("<unk>", word, voc, lm) - 8700)
						: (getbigramprob(voc.getWord(G_viterbi_map[j][cnt - 1]), word, voc, lm) + G_viterbi_prob[j][cnt - 1]);
					if (prob > max_prob)
					{
						max_prob = prob;
						max_prob_idx = j;
					}
				}
				G_viterbi_prob[i][cnt] = max_prob;
				G_viterbi_path[i][cnt] = max_prob_idx;
			}
		}
	}
	// 若這個字是國字
	else
	{
		// 若在測資該行的第一個字
		if (cnt == 0)
		{
			G_viterbi_prob[0][cnt] = getbigramprob("<unk>", pch, voc, lm);
			G_viterbi_path[0][cnt] = -1;
		}
		// 若不是在測資該行的第一個字
		//     G_viterbi_prob[0][cnt] = max_j(  getbigramprob(voc.getWord(G_viterbi_map[j][cnt-1]), word, voc, lm) + G_viterbi_prob[j][cnt-1]  )
		//     G_viterbi_path[0][cnt] = argmax_j(  getbigramprob(voc.getWord(G_viterbi_map[j][cnt-1]), pch, voc, lm) + G_viterbi_prob[j][cnt-1]  )
		else
		{
			double max_prob = -870000;
			int max_prob_idx = -1;
			for (int j = 0; j < G_ch_num[cnt - 1]; j++)
			{
				prob = (G_viterbi_map[j][cnt - 1] == -1)
					? (getbigramprob("<unk>", pch, voc, lm) - 8700)
					: (getbigramprob(voc.getWord(G_viterbi_map[j][cnt - 1]), pch, voc, lm) + G_viterbi_prob[j][cnt - 1]);
				if (prob > max_prob)
				{
					max_prob = prob;
					max_prob_idx = j;
				}
			}
			G_viterbi_prob[0][cnt] = max_prob;
			G_viterbi_path[0][cnt] = max_prob_idx;
		}
	}
}
