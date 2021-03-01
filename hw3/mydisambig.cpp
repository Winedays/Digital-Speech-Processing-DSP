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

// �p��  G_viterbi_out_ch [cnt] [0~2]
void get_G_viterbi_out_ch( int cnt, char *word)
{
	strncpy(G_viterbi_out_ch[cnt], word, 2);
}

// ��X���G
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
	test_data.open(testdata_file,ios::in ); // ���� 
	map_data.open(map_file,ios::in ); // �`��-��r map
	zy_table_data.open( "zy_table.txt" ,ios::in ); // �`����

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

	char text[870]; // �˾�檺�r 
	char word[2]; // �ˤ@�Ӧr 
	char *pch; // ���r�ɥΨӸ˦r
	int cnt; // Ū����ꪺ�ĴX�Ӧr�F
	int zy_idx; // �Ӧr�b�`������index (��r�h��-1)

	// �� 37 �Ӫ`����� G_zy_table[37][2]
	for(int i = 0 ; i < 37; i++)
	{
		zy_table_data >> G_zy_table[i];
	}

	// read file
	data_count = read_testdata(testdata_file, testdata);
	map_count = read_map(map_file, map);

	// ��� G_zy_ch_map[i] [8700], �Ҧp: G_zy_ch_map[2] = �v�}�Y���Ҧ���r
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

		// ���N����Y�檺�C�Ӧr, �M�κt��k
		pch = strtok(text, " ");
		while(pch != NULL)
		{
			// �ˬd�o�Ӧr�O���O�`��, �O�h�^�ǸӪ`���b�`������index, �_�h�^��-1
			zy_idx = is_zy(pch);

			// �p�� G_ch_num [cnt]
			get_G_ch_num(zy_idx, cnt);

			// �p�� G_viterbi_map [i][cnt] 
			get_G_viterbi_map(zy_idx, cnt, pch, voc);

			// �p�� G_viterbi_prob[i][cnt], G_viterbi_path[i][cnt]
			get_G_viterbi_prob_and_G_viterbi_path(zy_idx, cnt, pch, word, voc, lm);

			// �� G_viterbi_out_ch[cnt][0~2] //�����n�޳o��
			get_G_viterbi_out_ch(cnt, word);

			cnt++;

			pch = strtok(NULL, " ");
		}

		// ��X�Ӧ浲�G 
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
	// �Y�Q�J��Ů��~��Ū���AŪ�����㪺�@��ƾڡA�h�Φp�U�y�y�G
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
	// �Y�Q�J��Ů��~��Ū���AŪ�����㪺�@��ƾڡA�h�Φp�U�y�y�G
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


// �ˬd�o�Ӧr�O���O�`��, �O�h�^�ǸӪ`���b�`������ index, �_�h�^��-1
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

// �p�� G_ch_num [cnt]
// �Y�O��r, �N�O1
// �Y�O�`��, �N�O�Ӫ`�����X�� map �쪺��r��
void get_G_ch_num(int zy_idx, int cnt)
{
	G_ch_num[cnt] = (zy_idx == -1) ? 1 : (strlen(G_zy_ch_map[zy_idx]) / 3);
}


// �p�� G_viterbi_map [i][cnt]
// �Y�O��r, G_viterbi_map[0][cnt] = getIndex (�Ӧr)
// �Y�O�`��, G_viterbi_map[i][cnt] = getIndex (���Ӫ`�� map �쪺�� i �Ӱ�r)
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
			strncpy(word, G_zy_ch_map[zy_idx] + 3 * i + 1 , 2); // word �O�Ӫ`������i�Ӱ�r 
			strcpy(mapword, word ); // word �O�Ӫ`������i�Ӱ�r 
			mapword[2] = '\0' ;
			G_viterbi_map[i][cnt] = voc.getIndex(mapword);
			//printf("%s  ", mapword);
		}
	}
}

// �p�� G_viterbi_prob[i][cnt] �P G_viterbi_path[i][cnt]   ( i ���N�Ӫ`�����C�� map ����r ��index )
void get_G_viterbi_prob_and_G_viterbi_path(int zy_idx, int cnt, char *pch, char *word, Vocab &voc, Ngram &lm)
{
	double prob;
	// �Y�o�Ӧr�O�`��
	if (zy_idx >= 0)
	{
		for (int i = 0; i < strlen(G_zy_ch_map[zy_idx]) / 3; i++)
		{
			strncpy(word, G_zy_ch_map[zy_idx] + 3 * i + 1, 2); // word: �Ӫ`������i�Ӱ�r 

															   // �Y�b����Ӧ檺�Ĥ@�Ӧr(cnt == 0):
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
	// �Y�o�Ӧr�O��r
	else
	{
		// �Y�b����Ӧ檺�Ĥ@�Ӧr
		if (cnt == 0)
		{
			G_viterbi_prob[0][cnt] = getbigramprob("<unk>", pch, voc, lm);
			G_viterbi_path[0][cnt] = -1;
		}
		// �Y���O�b����Ӧ檺�Ĥ@�Ӧr
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
