/*
 *  huffman - Encode/Decode files using Huffman encoding.
 *  http://huffman.sourceforge.net
 *  Copyright (C) 2003  Douglas Ryan Richardson
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include<math.h>
#include "huffman.h"
#include<conio.h>
#ifdef WIN32
#include <winsock2.h>
#include <malloc.h>
#define alloca _alloca
#else
#include <netinet/in.h>
#endif
#define MAX_SYMBOLS 256
typedef struct huffman_node_tag
{
	unsigned char isLeaf;
	unsigned long count;
	struct huffman_node_tag *parent;

	union
	{
		struct
		{
			struct huffman_node_tag *zero, *one;
		};
		unsigned char symbol;
	};
} huffman_node;

typedef struct huffman_code
{
	/* The length of this code in bits. */
	unsigned long numbits;

	/* The bits that make up this code. The first
	   bit is at position 0 in bits[0]. The second
	   bit is at position 1 in bits[0]. The eighth
	   bit is at position 7 in bits[0]. The ninth
	   bit is at position 0 in bits[1]. */
	unsigned char *bits;
} huffman_code;


typedef struct SqStack_tag{
	char *base;
	char *top;
	int size;
}SqStack;



void Push(SqStack *s,char e){
	(*s).top++;
	*((*s).top)=e;
	(*s).size++;
	return;

}
char Pop(SqStack *s){

	char c;
	if((*s).size==0) 
		c='0';
	else {
	c=*((*s).top);
	(*s).top--;
	(*s).size--;
	}
	return c;
	

}

int IsEmpty(SqStack *s){
	if((*s).base==(*s).top) return 1;
	else
		return 0;
}





static unsigned long
numbytes_from_numbits(unsigned long numbits)
{
	return numbits / 8 + (numbits % 8 ? 1 : 0);
}
  
/*
 * get_bit returns the ith bit in the bits array
 * in the 0th position of the return value.
 */
static unsigned char
get_bit(unsigned char* bits, unsigned long i)
{
	return (bits[i / 8] >> i % 8) & 1;
}

static void
reverse_bits(unsigned char* bits, unsigned long numbits)
{
	unsigned long numbytes = numbytes_from_numbits(numbits);
	unsigned char *tmp =
	    (unsigned char*)alloca(numbytes);
	unsigned long curbit;
	long curbyte = 0;
	
	memset(tmp, 0, numbytes);

	for(curbit = 0; curbit < numbits; ++curbit)
	{
		unsigned int bitpos = curbit % 8;

		if(curbit > 0 && curbit % 8 == 0)
			++curbyte;
		
		tmp[curbyte] |= (get_bit(bits, numbits - curbit - 1) << bitpos);
	}

	memcpy(bits, tmp, numbytes);
}

/*
 * new_code builds a huffman_code from a leaf in
 * a Huffman tree.
 */
//生成霍夫曼码字
static huffman_code*
new_code(const huffman_node* leaf)
{
	/* Build the huffman code by walking up to
	 * the root node and then reversing the bits,
	 * since the Huffman code is calculated by
	 * walking down the tree. */
	unsigned long numbits = 0;
	unsigned char* bits = NULL;
	huffman_code *p;

	while(leaf && leaf->parent)
	{
		huffman_node *parent = leaf->parent;
		unsigned char cur_bit = (unsigned char)(numbits % 8);
		unsigned long cur_byte = numbits / 8;

		/* If we need another byte to hold the code,
		   then allocate it. */
		if(cur_bit == 0)
		{
			size_t newSize = cur_byte + 1;
			bits = (unsigned char*)realloc(bits, newSize);
			bits[newSize - 1] = 0; /* Initialize the new byte. */
		}

		/* If a one must be added then or it in. If a zero
		 * must be added then do nothing, since the byte
		 * was initialized to zero. */
		if(leaf == parent->one)
			bits[cur_byte] |= 1 << cur_bit;

		++numbits;
		leaf = parent;
	}

	if(bits)
		reverse_bits(bits, numbits);

	p = (huffman_code*)malloc(sizeof(huffman_code));
	p->numbits = numbits;
	p->bits = bits;
	return p;
}



//两个关键的数据结构
typedef huffman_node* SymbolFrequencies[MAX_SYMBOLS];
typedef huffman_code* SymbolEncoder[MAX_SYMBOLS];


//创建叶子节点
static huffman_node*
new_leaf_node(unsigned char symbol)
{
	huffman_node *p = (huffman_node*)malloc(sizeof(huffman_node));
	p->isLeaf = 1;
	p->symbol = symbol;
	p->count = 0;
	p->parent = 0;
	return p;
}

//创建非叶子节点
static huffman_node*
new_nonleaf_node(unsigned long count, huffman_node *zero, huffman_node *one)
{
	huffman_node *p = (huffman_node*)malloc(sizeof(huffman_node));
	p->isLeaf = 0;
	p->count = count;
	p->zero = zero;
	p->one = one;
	p->parent = 0;
	
	return p;
}

//释放霍夫曼树
static void
free_huffman_tree(huffman_node *subtree)
{
	if(subtree == NULL)
		return;

	if(!subtree->isLeaf)
	{
		free_huffman_tree(subtree->zero);
		free_huffman_tree(subtree->one);
	}
	
	free(subtree);
}

//释放霍夫曼码字
static void
free_code(huffman_code* p)
{
	free(p->bits);
	free(p);
}

//释放霍夫曼编码器
static void
free_encoder(SymbolEncoder *pSE)
{
	unsigned long i;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		huffman_code *p = (*pSE)[i];
		if(p)
			free_code(p);
	}

	free(pSE);
}

//初始化符号频率
static void
init_frequencies(SymbolFrequencies *pSF)
{
	memset(*pSF, 0, sizeof(SymbolFrequencies));
#if 0
	unsigned int i;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		unsigned char uc = (unsigned char)i;
		(*pSF)[i] = new_leaf_node(uc);
	}
#endif
}

//计算符号频率
static unsigned int
get_symbol_frequencies(SymbolFrequencies *pSF, FILE *in)
{
	int c;
	unsigned int total_count = 0;
	
	/* Set all frequencies to 0. */
	init_frequencies(pSF);
	
	/* Count the frequency of each symbol in the input file. */
	while((c = fgetc(in)) != EOF)
	{
		unsigned char uc = c;
		if(!(*pSF)[uc])
			(*pSF)[uc] = new_leaf_node(uc);
		++(*pSF)[uc]->count;
		++total_count;
	}

	return total_count;
}

/*
 * When used by qsort, SFComp sorts the array so that
 * the symbol with the lowest frequency is first. Any
 * NULL entries will be sorted to the end of the list.
 */
//比较符号频率
static int
SFComp(const void *p1, const void *p2)
{
	const huffman_node *hn1 = *(const huffman_node**)p1;
	const huffman_node *hn2 = *(const huffman_node**)p2;

	/* Sort all NULLs to the end. */
	if(hn1 == NULL && hn2 == NULL)
		return 0;
	if(hn1 == NULL)
		return 1;
	if(hn2 == NULL)
		return -1;
	
	if(hn1->count > hn2->count)
		return 1;
	else if(hn1->count < hn2->count)
		return -1;

	return 0;
}


#if 1
//打印符号频率
static void
print_freqs(SymbolFrequencies * pSF)
{
	size_t i;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		if((*pSF)[i])
			printf("%c, %ld\n", (*pSF)[i]->symbol, (*pSF)[i]->count);
		//else
		//	printf("NULL\n");
	}
}
#endif



/*
 * build_symbol_encoder builds a SymbolEncoder by walking
 * down to the leaves of the Huffman tree and then,
 * for each leaf, determines its code.
 */
//创建霍夫曼编码器
static void
build_symbol_encoder(huffman_node *subtree, SymbolEncoder *pSF)
{
	if(subtree == NULL)
		return;

	if(subtree->isLeaf)
		(*pSF)[subtree->symbol] = new_code(subtree);
	else
	{
		build_symbol_encoder(subtree->zero, pSF);
		build_symbol_encoder(subtree->one, pSF);
	}
}

void printHuffmanTree(huffman_node *subtree, SymbolFrequencies *pSF,int t){
	if(subtree == NULL)
		return;
	

	if(subtree->isLeaf)
		printf("%c\n",subtree->symbol);
	else
	{
		
		printf("*\n");
		for (int i = t; i >= 0; i--) {
			printf("\t");
		}
		printHuffmanTree(subtree->zero, pSF,t+1);
		for (int i = t; i >= 0; i--) {
			printf("\t");
		}
		printHuffmanTree(subtree->one, pSF,t+1);
	}
}

/*
void printHuffmanTree(SymbolFrequencies * pSF){
	SqStack *s;
	unsigned int i = 0;
	unsigned int n = 0;
	huffman_node *m1 = NULL, *m2 = NULL;
	char c1,c2;

	s=(SqStack *)malloc(MAX_SYMBOLS*sizeof(SqStack));
	(*s).base = (char *)malloc(sizeof(char));
	(*s).top=(*s).base;
	(*s).size=0;

	for(n = 0; n < MAX_SYMBOLS && (*pSF)[n]; ++n)
		;


	/*
	 * Construct a Huffman tree. This code is based
	 * on the algorithm given in Managing Gigabytes
	 * by Ian Witten et al, 2nd edition, page 34.
	 * Note that this implementation uses a simple
	 * count instead of probability.
	 */

	/*
	for(i = 0; i < n - 1; ++i)
	{
		/* Set m1 and m2 to the two subsets of least probability. */
		/*m1 = (*pSF)[0];
		m2 = (*pSF)[1];
		c1=m1->symbol;
		c2=m2->symbol;
		Push(s,c1);
		Push(s,c2);
		/* Replace m1 and m2 with a set {m1, m2} whose probability
		 * is the sum of that of m1 and m2. */
	/*	(*pSF)[0] = m1->parent = m2->parent =
			new_nonleaf_node(m1->count + m2->count, m1, m2);
		(*pSF)[1] = NULL;
		
		/* Put newSet into the correct count position in pSF. */
	/*	qsort((*pSF), n, sizeof((*pSF)[0]), SFComp);
		
	}
	
	while((*s).size>0){
		printf("%c\n",Pop(s));

	}


}*/
/*
 * calculate_huffman_codes turns pSF into an array
 * with a single entry that is the root of the
 * huffman tree. The return value is a SymbolEncoder,
 * which is an array of huffman codes index by symbol value.
 */
//创建霍夫曼树
static SymbolEncoder*
calculate_huffman_codes(SymbolFrequencies * pSF)
{
	unsigned int i = 0;
	unsigned int n = 0;
	huffman_node *m1 = NULL, *m2 = NULL;
	SymbolEncoder *pSE = NULL;
	
#if 0
	printf("BEFORE SORT\n");
	print_freqs(pSF);
#endif

	/* Sort the symbol frequency array by ascending frequency. */
	//对符号频率进行排序
	qsort((*pSF), MAX_SYMBOLS, sizeof((*pSF)[0]), SFComp);

#if 0	
	printf("AFTER SORT\n");
	print_freqs(pSF);
#endif



	/* Get the number of symbols. */
	for(n = 0; n < MAX_SYMBOLS && (*pSF)[n]; ++n)
		;

	/*
	 * Construct a Huffman tree. This code is based
	 * on the algorithm given in Managing Gigabytes
	 * by Ian Witten et al, 2nd edition, page 34.
	 * Note that this implementation uses a simple
	 * count instead of probability.
	 */

	for(i = 0; i < n - 1; ++i)
	{
		/* Set m1 and m2 to the two subsets of least probability. */
		m1 = (*pSF)[0];
		m2 = (*pSF)[1];

		/* Replace m1 and m2 with a set {m1, m2} whose probability
		 * is the sum of that of m1 and m2. */
		(*pSF)[0] = m1->parent = m2->parent =
			new_nonleaf_node(m1->count + m2->count, m1, m2);
		(*pSF)[1] = NULL;
		
		/* Put newSet into the correct count position in pSF. */
		qsort((*pSF), n, sizeof((*pSF)[0]), SFComp);
	}

	/* Build the SymbolEncoder array from the tree. */
	pSE = (SymbolEncoder*)malloc(sizeof(SymbolEncoder));
	memset(pSE, 0, sizeof(SymbolEncoder));
	build_symbol_encoder((*pSF)[0], pSE);
	return pSE;
}

/*
 * Write the huffman code table. The format is:
 * 4 byte code count in network byte order.
 * 4 byte number of bytes encoded
 *   (if you decode the data, you should get this number of bytes)
 * code1
 * ...
 * codeN, where N is the count read at the begginning of the file.
 * Each codeI has the following format:
 * 1 byte symbol, 1 byte code bit length, code bytes.
 * Each entry has numbytes_from_numbits code bytes.
 * The last byte of each code may have extra bits, if the number of
 * bits in the code is not a multiple of 8.
 */


//输出码表
static int
write_code_table(FILE* out, SymbolEncoder *se, unsigned int symbol_count)
{
	unsigned long i, count = 0;
	
	/* Determine the number of entries in se. */
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		if((*se)[i])
			++count;
	}

	/* Write the number of entries in network byte order. */
	i = htonl(count);
	if(fwrite(&i, sizeof(i), 1, out) != 1)
		return 1;

	/* Write the number of bytes that will be encoded. */
	symbol_count = htonl(symbol_count);
	if(fwrite(&symbol_count, sizeof(symbol_count), 1, out) != 1)
		return 1;

	/* Write the entries. */
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		huffman_code *p = (*se)[i];
		if(p)
		{
			unsigned int numbytes;
			/* Write the 1 byte symbol. */
			fputc((unsigned char)i, out);
			/* Write the 1 byte code bit length. */
			fputc(p->numbits, out);
			/* Write the code bytes. */
			numbytes = numbytes_from_numbits(p->numbits);
			if(fwrite(p->bits, 1, numbytes, out) != numbytes)
				return 1;
		}
	}

	return 0;
}


/*
 * read_code_table builds a Huffman tree from the code
 * in the in file. This function returns NULL on error.
 * The returned value should be freed with free_huffman_tree.
 */
//读取码表
static huffman_node*
read_code_table(FILE* in, unsigned int *pDataBytes)
{
	huffman_node *root = new_nonleaf_node(0, NULL, NULL);
	unsigned int count;
	
	/* Read the number of entries.
	   (it is stored in network byte order). */
	if(fread(&count, sizeof(count), 1, in) != 1)
	{
		free_huffman_tree(root);
		return NULL;
	}

	count = ntohl(count);

	/* Read the number of data bytes this encoding represents. */
	if(fread(pDataBytes, sizeof(*pDataBytes), 1, in) != 1)
	{
		free_huffman_tree(root);
		return NULL;
	}

	*pDataBytes = ntohl(*pDataBytes);


	/* Read the entries. */
	while(count-- > 0)
	{
		int c;
		unsigned int curbit;
		unsigned char symbol;
		unsigned char numbits;
		unsigned char numbytes;
		unsigned char *bytes;
		huffman_node *p = root;
		
		if((c = fgetc(in)) == EOF)
		{
			free_huffman_tree(root);
			return NULL;
		}
		symbol = (unsigned char)c;
		
		if((c = fgetc(in)) == EOF)
		{
			free_huffman_tree(root);
			return NULL;
		}
		
		numbits = (unsigned char)c;
		numbytes = (unsigned char)numbytes_from_numbits(numbits);
		bytes = (unsigned char*)malloc(numbytes);
		if(fread(bytes, 1, numbytes, in) != numbytes)
		{
			free(bytes);
			free_huffman_tree(root);
			return NULL;
		}

		/*
		 * Add the entry to the Huffman tree. The value
		 * of the current bit is used switch between
		 * zero and one child nodes in the tree. New nodes
		 * are added as needed in the tree.
		 */
		for(curbit = 0; curbit < numbits; ++curbit)
		{
			if(get_bit(bytes, curbit))
			{
				if(p->one == NULL)
				{
					p->one = curbit == (unsigned char)(numbits - 1)
						? new_leaf_node(symbol)
						: new_nonleaf_node(0, NULL, NULL);
					p->one->parent = p;
				}
				p = p->one;
			}
			else
			{
				if(p->zero == NULL)
				{
					p->zero = curbit == (unsigned char)(numbits - 1)
						? new_leaf_node(symbol)
						: new_nonleaf_node(0, NULL, NULL);
					p->zero->parent = p;
				}
				p = p->zero;
			}
		}
		
		free(bytes);
	}

	return root;
}

//根据生成的霍夫曼码表，对输入文件逐个字符进行霍夫曼编码，并将编码结果输出
static int
do_file_encode(FILE* in, FILE* out, SymbolEncoder *se)
{
	unsigned char curbyte = 0;
	unsigned char curbit = 0;
	int c;
	
	while((c = fgetc(in)) != EOF)
	{
		unsigned char uc = (unsigned char)c;
		huffman_code *code = (*se)[uc];
		unsigned long i;
		
		for(i = 0; i < code->numbits; ++i)
		{
			/* Add the current bit to curbyte. */
			curbyte |= get_bit(code->bits, i) << curbit;

			/* If this byte is filled up then write it
			 * out and reset the curbit and curbyte. */
			if(++curbit == 8)
			{
				fputc(curbyte, out);
				curbyte = 0;
				curbit = 0;
			}
		}
	}

	/*
	 * If there is data in curbyte that has not been
	 * output yet, which means that the last encoded
	 * character did not fall on a byte boundary,
	 * then output it.
	 */
	if(curbit > 0)
		fputc(curbyte, out);

	return 0;
}
//计算频率
void getfrequence(SymbolFrequencies * pSF,int total,float f[MAX_SYMBOLS]){
	int i;

	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		f[i]=0;
		if((*pSF)[i])
			f[i]=(float)(*pSF)[i]->count/total;
	}
}

//计算熵
void calcEntropy(SymbolFrequencies * pSF,float f[MAX_SYMBOLS]){
	int i;

	float shang=0;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		if((*pSF)[i])
		{
			shang+=(float)(f[i]*(log(1/f[i])/log(2)));
		}
	
	}
	printf("熵:%.3f\n",shang);

}
//计算平均编码符号长度
void calcAvgSymbolLength(SymbolEncoder *se,float f[MAX_SYMBOLS]){
	size_t i;

	float wlength=0;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		if((*se)[i])
		{
		
			wlength+=(f[i]*((*se)[i]->numbits));
		}
	
	}
	printf("平均编码符号长度:%.2f\n",wlength);
}

void calcCompressionRatio(int total,float f[MAX_SYMBOLS],SymbolEncoder *se){

	long inbits=total*8;
	long outbits=0;
	float rate;
	int i;
	for(i = 0; i < MAX_SYMBOLS; ++i)
	{
		if((*se)[i])
		{
		
			outbits=(long)((*se)[i]->numbits*total*f[i]);
		}
	
	}

	rate=((float)(inbits-outbits)/(float)inbits);

	printf("压缩比:%.3f\n",rate);


}



/*
 * huffman_encode_file huffman encodes in to out.
 */
//霍夫曼编码
int
huffman_encode_file(FILE *in, FILE *out)
{
	SymbolFrequencies sf;
	SymbolEncoder *se;
	huffman_node *root = NULL;
	int rc;
	unsigned int symbol_count;
	//计算频率
	float frequence[MAX_SYMBOLS];


	/* Get the frequency of each symbol in the input file. */
	//获取符号频率
	symbol_count = get_symbol_frequencies(&sf, in);

	//计算各个字符概率
	getfrequence(&sf,symbol_count,frequence);
	//计算熵
	calcEntropy(&sf,frequence);


	/* Build an optimal table from the symbolCount. */
	//创建霍夫曼树
	se = calculate_huffman_codes(&sf);
	root = sf[0];

	//凹入表打印霍夫曼树
	printf("凹入表打印如下：\n");
	printHuffmanTree(root,&sf,0);

	//计算平均编码符号长度
	calcAvgSymbolLength(se,frequence);

	//计算压缩比
	calcCompressionRatio(symbol_count, frequence, se);

	/* Scan the file again and, using the table
	   previously built, encode it into the output file. */
	rewind(in);



	//输出霍夫曼码表
	rc = write_code_table(out, se, symbol_count);

	//对输入文件进行霍夫曼编码
	if(rc == 0)
		rc = do_file_encode(in, out, se);




	/* Free the Huffman tree. */
	free_huffman_tree(root);
	free_encoder(se);
	return rc;
}

//霍夫曼解码

int
huffman_decode_file(FILE *in, FILE *out)
{
	huffman_node *root, *p;
	int c;
	unsigned int data_count;
	
	/* Read the Huffman code table. */

	root = read_code_table(in, &data_count);
	if(!root)
		return 1;

	/* Decode the file. */
	p = root;
	while(data_count > 0 && (c = fgetc(in)) != EOF)
	{
		unsigned char byte = (unsigned char)c;
		unsigned char mask = 1;
		while(data_count > 0 && mask)
		{
			p = byte & mask ? p->one : p->zero;
			mask <<= 1;

			if(p->isLeaf)
			{
				fputc(p->symbol, out);
				p = root;
				--data_count;
			}
		}
	}

	free_huffman_tree(root);
	return 0;
}



