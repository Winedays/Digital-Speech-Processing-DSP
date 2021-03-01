# -*- coding: utf-8 -*-
import sys
def readfile( file ):
    text = open( file  ,"r" , encoding='big5-hkscs' )
    big5_list = []
    zhuyin2big5_dist = {}
    for row in text :
        big5 , zhuyins = row.split(' ')
        zhuyin_list = zhuyins.split('/')
        # mapping each zhuyin to big5
        for zy in zhuyin_list :
            if zy[0] in zhuyin2big5_dist : # 只看第一個音
                if big5 not in zhuyin2big5_dist[ zy[0] ] : # 只要沒被記錄過的
                    zhuyin2big5_dist[ zy[0] ] += ' ' + big5
            else :
                zhuyin2big5_dist[ zy[0] ] = big5
        # save each big5
        big5_list.append( big5 )
    for big5 in big5_list :
        # add each big5 to mapping
        zhuyin2big5_dist[ big5 ] = big5
    text.close()
    return zhuyin2big5_dist

def savefile( filepath , dist ):
    file = open( filepath , "w" , encoding='big5-hkscs' )
    for key, value in dist.items() :
        file.write( str(key) + '\t' + str(value) + '\n' )
    file.close()
        
if __name__ == "__main__":
    zhuyin2big5_dist = readfile( sys.argv[1] )
    savefile( sys.argv[2] , zhuyin2big5_dist )
