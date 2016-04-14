# Huffman
Created by xujin

Completed by xiaoFine in 'Huffman Souring Encoding Comprehensive Experiment'

  

Huffman编码属于熵编码的方法之一，是根据信源符号出现概率的分布特性而进行的压缩编码。
Huffman编码的主要思想是：出现概率大的符号用短的码字表示；反之，出现概率小的符号用长的码字表示。
Huffman编码过程描述：
1. 初始化:
将信源符号按出现频率(或概率)进行递增顺序排列，输入集合L;
2. 重复如下操作直至L中只有1个节点:
(a) 从L中取得两个具有最低频率(或概率)的节点，为它们创建一个父节点； 
(b) 将它们的频率和(或概率和)赋给父结点，并将其插入L;
3. 进行编码 ：
    从根节点开始，左子节点赋予1，右节点赋予0，直到叶子节点。
【基本定义】
1.	熵
熵是信息量的度量方法，它表示某一事件出现的概率越小，则该事件包含的信息就越多。根据Shannon理论，信源S的熵定义为 ,其中 是符号 在S中出现的概率。
2.	平均编码符号长度
假设符号 编码后长度为li (i=1,…,n)，则平均编码符号长度L为： 
3.	压缩比
设原始字符串的总长度为Lorig位,编码后的总长度为Lcoded位，则压缩比R为
       R = (Lorig - Lcoded)/ Lorig
4.	凹入表
下图为一棵示例用的二叉树：
 
在计算机中，上述二叉树可通过如下的凹入表表示：
a
	b
		d
		e
	c
		f
		g

在调试过程中，观察两个关键变量SymbolFrequencies sf，SymbolEncoder *se的变化。
SymbolFrequencies的定义：
typedef huffman_node* SymbolFrequencies[MAX_SYMBOLS]；
SymbolEncoder的定义：
typedef huffman_code* SymbolEncoder[MAX_SYMBOLS];

而huffman_node和huffman_code是两个结构，分别用来记录霍夫曼编码树节点和霍夫曼码字信息。
typedef struct huffman_node_tag
{
	unsigned char isLeaf; //是否叶子节点
	unsigned long count; //源符号出现的次数
		struct huffman_node_tag *parent; //父节点指针
 	union
		{
			struct
			{
				struct huffman_node_tag *zero, *one; 
//子节点指针，zero表示编码时赋0，one表示编码时赋1
			};
			unsigned char symbol; //源符号
		};
} huffman_node;
		/****************************/
typedef struct huffman_code_tag
{
		/* The length of this code in bits. */
		unsigned long numbits; //以bit为单位的码字长度
		unsigned char *bits; //码字串
} huffman_code;


