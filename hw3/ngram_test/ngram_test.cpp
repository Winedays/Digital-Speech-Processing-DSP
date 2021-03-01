#include <stdio.h>
#include "Ngram.h"

int main(int argc, char *argv[])
{
    int ngram_order = 3;
    Vocab voc;
    Ngram lm( voc, ngram_order );

    {
        const char lm_filename[] = "./corpus.lm";
        File lmFile( lm_filename, "r" );
        lm.read(lmFile);
        lmFile.close();
    }

    VocabIndex wid = voc.getIndex("Ê¨");
    if(wid == Vocab_None) {
        printf("No word with wid = %d\n", wid);
        printf("where Vocab_None is %d\n", Vocab_None);
    }

    wid = voc.getIndex("±wªÌ");
    VocabIndex context[] = {voc.getIndex("Å}") , voc.getIndex("¬r"), Vocab_None};
    printf("Log Prob(±wªÌ|¬r-Å}) = %f\n", lm.wordProb(wid, context));

    return 0;
}

