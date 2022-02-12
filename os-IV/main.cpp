#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <fstream>
#include<sstream>
using namespace std;
const char *RED = "\033[31m";
const char *WHITE = "\033[0m";
//输出缓存区
char printBuffer[100000];
//声明要使用的汇编函数，告诉编译器使用c规范来链接，否则有名空间，找不到函数。
//注意：这个print只能打印字符串，当打印数字时应使用std::to_string().c_str()将数字转换为c字符串。
extern "C" {
	void print(const char* printBuffer);
}
//每次输出之前要把Buffer清空。
void initPrintBuffer() {
	for (int i = 0;i < 100000;i++) {
		printBuffer[i] = 0;
	}
}
//打印红字:即先打印红色字符串，而后打印字，最后打印白色字符串。
void printRed(const char * str) {
	initPrintBuffer();
	strcpy(printBuffer, RED);
	strcat(printBuffer, str);
	strcat(printBuffer, WHITE);
	print(printBuffer);
	initPrintBuffer();
}



//下面这部分开始定义数据结构。
class File;
class Item {
public:
	// 读入所有子项
	virtual vector<File*> getSubItems() = 0;
	// 表示当前是否是一个目录
	virtual bool isFolder() = 0;
	// 小端操作系统读入从str + offset，长度为length的整数值
	unsigned getIntValue(const char *str, int offset = 0, int length = 1) {
		unsigned result = 0;
		for (int i = 0; i < length; i++, offset++) {
			// 首先 8 * i 计算出左移数字，然后偏移对应数字，最后加上去
			result += (unsigned char)(str[offset]) << i * 8;
		}
		return result;
	}
};

class File : public Item {

public:
	// 名字
	string name;
	// 扩展名
	string ext;
	// 读入是不是特殊项
	bool isSpecial;
	// 是否是文件夹
	bool isDir;
	// 最后一次写入时间
	int lastTime;
	// 最后一次写入日期
	int lastDate;
	// 内容指针
	const char *contentPointer;
	// 文件大小
	int size;
	// 数据区开始指针
	const char* dataSectors;
	// FAT对应指针位置
	const char* FATSector;
	// 开始簇号
	int startClusterNum;

	// 初始化表
	File(const char *start, const char *dataSectors, const char* FATSector)
		: dataSectors(dataSectors), FATSector(FATSector)
	{
		// 读入文件名
		string name(start, 8);
		for (char i : name) {
			if (i != ' ') {
				this->name += i;
			}
		}
		// 读入文件扩展名
		string ext(start + 8, 3);
		for (char i : ext) {
			if (i != ' ') {
				this->ext += i;
			}
		}
		// 读入是否是文件夹
		isDir = getIntValue(start, 0xB, 1) == 0x10;
		// 读入最后一次写入时间
		lastTime = getIntValue(start, 0x16, 2);
		// 读入最后一次写入日期
		lastDate = getIntValue(start, 0x18, 2);
		// 读入文件大小
		size = getIntValue(start, 0x1C, 4);
		// 读入此条对应的开始簇数
		startClusterNum = getIntValue(start, 0x1A, 2);
		// 读入当前的内容指针
		contentPointer = dataSectors + (startClusterNum - 2) * 512;
		// 读入是不是特殊项，比如本级文件夹.、上级文件夹..
		isSpecial = start[0] == '.';
	}

	// 读入全称
	string getFullName() {
		return isDir ? name: name + '.' + ext;
	}

	// 读入是否是文件夹
	bool isFolder() override {
		return isDir;
	}

	// 读入子项，只有是文件夹时才进行操作，不是文件夹则返回空
	vector<File*> getSubItems() override {
		vector<File*> subItems;
		if (isDir) {
			for (int i = 0;; i += 0x20) {
				if (contentPointer[i] != 0) {
					subItems.push_back(new File(
						contentPointer + i, dataSectors, FATSector));
				}
				else {
					break;
				}
			}
		}
		return subItems;
	};
	// 读取内容
	string readContent()
	{
		string fileContent;
		unsigned clusterNo = startClusterNum;
		// 当簇号大于等于0xFF8表示已经达到最后一簇
		while (clusterNo < 0xFF8) {
			if (clusterNo >= 0xFF0) {
				throw "encountered bad cluster";
			}
			// 当前的数据区指针
			const char* contentPointer = dataSectors + (clusterNo - 2) * 512;
			fileContent += string(contentPointer, 512);
			// 找到对应的下一个簇号
			clusterNo = nextClusterNo(clusterNo);
		}

		return fileContent.substr(0, size);
	}

	void printItem() {
		if (isDir) {
			printRed(getFullName().c_str());
		}
		else {
			print(getFullName().c_str());
		}
	}

private:
	// 读取之后从FAT中拿到下一簇，一个簇号1.5 字节
	unsigned nextClusterNo(unsigned current) {
		bool isOdd = current % 2 == 1;
		unsigned bytes = getIntValue(FATSector, current / 2 * 3, 3); 
		if (isOdd) {
			return bytes >> (3 * 4); // 获得高1.5 bytes
		}
		else {
			return bytes & 0x000FFF; // 标记高1.5 bytes
		}
	}
};

// Image
class Image : public Item {
public:
	char *content;
	Image(const char *file) {
		const int size = 1440 * 1024;
		this->content = new char[size];
		ifstream in(file, ios::binary);
		in.read(this->content, size);
		in.close();
	}
	// 覆盖写为True
	bool isFolder() override {
		return true;
	}

	// 读入所有的子项
	vector<File*> getSubItems() override {
		vector<File*> rootItems;
		// 拿到根目录区的指针
		char* rootPointer = RootDirSectors();
		for (int i = 0;; i += 0x20) {
			if (rootPointer[i] != 0) {
				// 找到名字开始的地方，每一个目录项的大小为0x20字节，所以加0x20
				rootItems.push_back(
					new File(rootPointer + i, DataSectors(), FATSector()));
			}
			else {
				break;
			}
		}
		return rootItems;
	}
	// FAT表读取
	char *FATSector()
	{
		// boot分区占据的扇区数
		const int ResvdSecCnt = getIntValue(content, 14, 2);
		return content + (ResvdSecCnt * 512); // sector 1
	}

	// 根目录区读取
	char *RootDirSectors() {
		// boot分区占据的扇区数
		const int ResvdSecCnt = getIntValue(content, 14, 2);
		// 总共的FAT的表的数量
		const int NumFATs = getIntValue(content, 16, 1);
		// 一个FAT的占位大小
		const int FATSz16 = getIntValue(content, 22, 2);
		// 9 Sections for FAT1，9 Sections for FAT2
		return content + ((ResvdSecCnt + NumFATs * FATSz16) * 512);
	}

	// 数据区读取
	char *DataSectors() {
		// 每扇区字节数（Bytes/Sector）
		const int BytePerSec = getIntValue(content, 11, 2);
		// boot分区占据的扇区数
		const int ResvdSecCnt = getIntValue(content, 14, 2);
		// 总共的FAT的表的数量
		const int NumFATs = getIntValue(content, 16, 1);
		// 根目录区文件最大数
		const int BPB_RootEntCnt = getIntValue(content, 17, 2);
		// 一个FAT的占位大小
		const int FATSz16 = getIntValue(content, 22, 2);
		// 根目录区扇区数量，根目录区由目录项组成，一个目录项大小为32字节
		const int RootDirSectors = ((BPB_RootEntCnt * 32) + (BytePerSec - 1)) / BytePerSec;
		return content + ((ResvdSecCnt + NumFATs * FATSz16 + RootDirSectors) * 512);
	}

	~Image() {
		delete content;
	}

};







//上面基本的数据结构都定义完了，该进行指令的解析和处理了。
//声明会用到的函数和变量
Image image("./a.img");
void print_As_Ordered(int FolderCount, int fileCount);
void print_As_Ordered(std::string parent, int FolderCount, int fileCount);
vector<string> split(string str, char separator);
vector<string> commandSplit(string command);
Item* findItem(Item* root, vector<string>& dirs, int now);
string formalizePath(string ori_path);
void lsOutput(Item* dir, string parent);
void ls(string path);
void count_print_folder_detail(Item* root, string path, int type);
void ls_lOuptut(Item* dir, string parent);
void ls_l(string path);
void cat(string path);
void input();





//这里是格式化输出的一个处理。
// 非文件夹时，输出文件数和目录数两个数字。
void print_As_Ordered(int FolderCount, int fileCount) {
	initPrintBuffer();
	strcpy(printBuffer, "  ");
	strcat(printBuffer, std::to_string(FolderCount).c_str());
	strcat(printBuffer, " ");
	strcat(printBuffer, std::to_string(fileCount).c_str());
	strcat(printBuffer,"\n");
	print(printBuffer);
	initPrintBuffer();
}

// 以文件夹的形式输出,如果到了一个新的文件夹，还要先输出目录名。
void print_As_Ordered(std::string parent, int FolderCount, int fileCount) {
	initPrintBuffer();
	strcpy(printBuffer, parent.c_str());
	strcat(printBuffer, " ");
	strcat(printBuffer, std::to_string(FolderCount).c_str());
	strcat(printBuffer, " ");
	strcat(printBuffer, std::to_string(fileCount).c_str());
	strcat(printBuffer, ": \n");
	print(printBuffer);
	initPrintBuffer();
}

/*
按照Separator来切分str，这个函数用于路径的分割。
*/
vector<string> split(string str, char separator){
	vector<string> res;
	string temp;
	for (char c : str){
		if (c == separator){
			if (temp.length() > 0){
				res.push_back(temp);
			}
			temp.clear();
		}
		else{
			temp += c;
		}
	}
	if (temp.length() > 0){
		res.push_back(temp);
	}
	return res;
}

/*
command为输入的一行命令
切分命令，返回数组
vector<string>为返回的切分后的命令
*/
//这里利用的方法类似提取句子中的所有单词，如果是-开头的能和-l通配的就记为-l
vector<string> commandSplit(string command) {
	vector <string>params;
	string thisparam = "";
	istringstream is(command);
	while (is >> thisparam) {
		if (regex_match(thisparam, regex("-(l)+"))) {
			params.push_back("-l");
		}
		else params.push_back(thisparam);
	}
	return params;
	}




/*
	查找对应的Item，从Root开始，根据parents各级根目录路径中进行查找，now 指的是现在查询到了目录的第几级。
*/
Item* findItem(Item* root, vector<string>& parents, int now) {
	if (root == nullptr) {
		throw "Sorry, I cannot find this item";
	}
	//如果查询到了目录的最后一级，返回根即可。
	if (now == parents.size()) {
		return root;
	}
	else {
		// 进行广度有限搜索，搜索目录的所有子项。如果搜索到了这一级，就以它为根，搜索目录的下一级。
		for (File* item : root->getSubItems()) {
			if (item->getFullName() == parents[now]) {
				return findItem(item, parents, now + 1);
			}
		}
		throw "Sorry, I cannot find this item";
	}
}



//补齐首位可能遗漏的/(注：其实开头的\是不能遗漏的，否则就是无效地址了。但结尾的\是可以遗漏的。
string formalizePath(string ori_path) {
	string res = "" + ori_path;
	if (res[0] != '/') {
		res = '/' + res;
	}
	if (res[res.length() - 1] != '/') {
		res += '/';
	}
	return res;
}



/*
ls 的输出
*/
void lsOutput(Item* root, string parent) {
	// 首先输出路径，加冒号，换行。
	print(parent.c_str());
	print(":\n");
	// 输出所有的内容
	//这里不知道其子项究竟是文件还是目录，因此用一个auto，好做一个多态。
	auto subItems = root->getSubItems();
	for (auto item : subItems) {
		//多态，各自输出各自的文件名。
		item->printItem();
		//要求的格式
		print("  ");
	}
	print("\n");
	//如果有子目录，再递归输出子目录的相关信息。
	for (auto item : subItems) {
		if (item->isDir && !item->isSpecial) {
			lsOutput(item, parent + item->name + '/');
		}
	}
}

/*
ls 无-l指令
*/
void ls(string path){
	//先将字符串首位可能遗漏的/都补齐全。
	path = formalizePath(path);
	//提取各级子目录。
	vector<string> parents = split(path, '/');
	//将指针指向要找的这一级item，最初的root就是image根目录的root，最初得层数要从第一级开始找。
	Item *root = findItem(&image, parents, 0);
	//如果是一个目录，就按目录进行输出
	if (root->isFolder()) {
		lsOutput(root, path);
	}
	//如果是一个文件，直接扔出报错信息即可。
	else {
		throw "Sorry, it is a file.";
	}
}
void count_print_folder_detail(Item* root,string path,int type) {
	//type为1表示打印的是每个第一行的根目录的信息，要完整路径名称的
	//type为2表示打印的是红色的目录名加两个数字那种介绍信息。
	auto subItems = root->getSubItems();
	int fileCount = 0, FolderCount = 0;
	for (auto item : subItems) {
		// . & .. 不记录
		//这里要统计所有的文件数和文件夹数。
		if (item->isDir && !item->isSpecial) {
			FolderCount++;
		}
		else if (!item->isDir) {
			fileCount++;
		}
	}
	if(type==1){ print_As_Ordered(path, FolderCount, fileCount); }
	if(type==2){
		File* thisroot = (File*)root;
		printRed(thisroot->name.c_str());
		//如果是.和..就输出名字然后换行就行了。
		if(thisroot->isDir && !thisroot->isSpecial)print_As_Ordered(FolderCount, fileCount);
		else
		{
			print("\n");
		}
	}
	return;
}




void ls_lOuptut(Item* root, string parent) {
	//这里的parent指的是完整的全路径。
	// step1:统计所有的文件数和文件夹数，输出第一行基础信息。
	auto subItems = root->getSubItems();
	count_print_folder_detail(root, parent, 1);


	//step2:输出所有子文件夹和子文件的基本信息（各自一行）
	// 循环输出当前目录内容
	for (auto item : subItems) {
		// 文件输出文件名和文件大小即可，其中，文件大小在ls_l里面输出的，所以要调用ls_l。
		if (!item->isFolder()) {
			ls_l(parent + item->getFullName());
			print("\n");
		}
		else {
			// 如果子项目是文件夹，需要输出直接子目录和直接子文件数。因此要以item为根重新统计该子目录的直接子目录数和直接子文件数。
			count_print_folder_detail(item, parent, 2);
		}
	}
	print("\n");
	// step3:递归输出本文件夹中文件夹的情况
	for (auto item : subItems) {
		if (item->isDir && !item->isSpecial) {
			ls_lOuptut(item, parent + item->name + '/');
		}
	}
}

/*
ls -l指令
*/
void ls_l(string path) {

	vector<string> parents = split(path, '/');
	Item *root = findItem(&image, parents, 0);
//如果是文件，按格式输出文件名和文件长度就结束了。
	if (!root->isFolder()) {
		initPrintBuffer();
		strcpy(printBuffer, (((File*)root)->name + "." + ((File*)root)->ext).c_str());
		strcat(printBuffer, "  ");
		strcat(printBuffer, to_string(((File*)root)->readContent().length()).c_str());
		print(printBuffer);
		initPrintBuffer();
		return;
	}
	path = formalizePath(path);
//如果是文件夹，就要开始一层一层的输出了。
	ls_lOuptut(root, path);
}

/*
cat命令
*/
void cat(string path) {
	//先把各级根目录都提取出来。
	vector<string> parents = split(path, '/');
	Item *root = findItem(&image, parents, 0);
	//
	if (root->isFolder()) {
		throw "This is a folder not a file.";
	}
	print(((File*)root)->readContent().c_str());
}

/*
	这里是处理每一行的输入调用相应的方法
*/
void input() {

	string input;
	getline(cin, input);//这里是一次读入一行指令，直接忽略里面的空格。
	vector<string> commandList = commandSplit(input);
	//由于上一个指令只输入了错误信息，因此这条指令要补上换行符。
	print("\n");
//如果输入为空，就什么也不做。
	if (commandList.size() == 0) {
		return;
	}
	// ls 命令处理
	if (commandList[0] == "ls") {
		// 没有更多参数
		if (commandList.size() == 1) {
			ls("/");
		}
		else {
			//这里开始判断ls指令是否合法了。
			int target = 0;
			//target表示要输出的目标地址或文件的数目。
			string dict = "";
			int hasL = 0;
			//hasL标注的是所有的参数里是否有-l。
			for (int i = 1; i < commandList.size(); i++) {
				if (commandList[i] == "-l")hasL = 1;
				//如果不是-l的话，可能是目标地址，用dict记录目标地址，用target记录目标地址的数目。
				else if (regex_match(commandList[i], regex("/[A-Z0-9./]+")) || commandList[i] == "/") {
					target++;
					dict = commandList[i];
				}
				else throw "Sorry, you entered the wrong param in ls command.";
			}
			// 限制只能打印一个
			if (target >= 2)throw "Sorry,there are too many files to ls";
			// 如果没有-l参数，就是ls address
			if (hasL == 0) {
				ls(commandList[1]);
			}
			//如果有-l参数，有目标地址从目标地址开始，没有目标地址从根目录开始。
			else {
				if (target == 1) {
					ls_l(dict);
				}
				else {
					ls_l("/");
				}
			}
		}
	}
	// cat 命令
	else if (commandList[0] == "cat") {
		//如果没有参数
		if (commandList.size() == 1) {
			throw "Sorry, the input isnot a path of file";
		}
		//如果参数过多。
		else if (commandList.size() > 2) {
			throw "Sorry,there are too many files to cat";
		}
		else {
			cat(commandList[1]);
		}
	}
	// exit命令
	else if (commandList[0] == "exit"&&commandList.size() == 1) {
		throw 0;
	}
	// 未知命令
	else {
		throw "Sorry,this command does not exist";
	}
}

int main(){
	while (true) {
		print("please enter your command:\n");
		try {
			input();
		}
		catch (const char* errorinfo) {
			print(errorinfo);
		}
		//所有的异常都被捕获了，只有返回了一个数字才会退出。
		catch (int exitCode) {
			// 没什么问题就返回0正常退出
			return exitCode;
		}
		print("\n");
	}
}