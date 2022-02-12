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
//���������
char printBuffer[100000];
//����Ҫʹ�õĻ�ຯ�������߱�����ʹ��c�淶�����ӣ����������ռ䣬�Ҳ���������
//ע�⣺���printֻ�ܴ�ӡ�ַ���������ӡ����ʱӦʹ��std::to_string().c_str()������ת��Ϊc�ַ�����
extern "C" {
	void print(const char* printBuffer);
}
//ÿ�����֮ǰҪ��Buffer��ա�
void initPrintBuffer() {
	for (int i = 0;i < 100000;i++) {
		printBuffer[i] = 0;
	}
}
//��ӡ����:���ȴ�ӡ��ɫ�ַ����������ӡ�֣�����ӡ��ɫ�ַ�����
void printRed(const char * str) {
	initPrintBuffer();
	strcpy(printBuffer, RED);
	strcat(printBuffer, str);
	strcat(printBuffer, WHITE);
	print(printBuffer);
	initPrintBuffer();
}



//�����ⲿ�ֿ�ʼ�������ݽṹ��
class File;
class Item {
public:
	// ������������
	virtual vector<File*> getSubItems() = 0;
	// ��ʾ��ǰ�Ƿ���һ��Ŀ¼
	virtual bool isFolder() = 0;
	// С�˲���ϵͳ�����str + offset������Ϊlength������ֵ
	unsigned getIntValue(const char *str, int offset = 0, int length = 1) {
		unsigned result = 0;
		for (int i = 0; i < length; i++, offset++) {
			// ���� 8 * i ������������֣�Ȼ��ƫ�ƶ�Ӧ���֣�������ȥ
			result += (unsigned char)(str[offset]) << i * 8;
		}
		return result;
	}
};

class File : public Item {

public:
	// ����
	string name;
	// ��չ��
	string ext;
	// �����ǲ���������
	bool isSpecial;
	// �Ƿ����ļ���
	bool isDir;
	// ���һ��д��ʱ��
	int lastTime;
	// ���һ��д������
	int lastDate;
	// ����ָ��
	const char *contentPointer;
	// �ļ���С
	int size;
	// ��������ʼָ��
	const char* dataSectors;
	// FAT��Ӧָ��λ��
	const char* FATSector;
	// ��ʼ�غ�
	int startClusterNum;

	// ��ʼ����
	File(const char *start, const char *dataSectors, const char* FATSector)
		: dataSectors(dataSectors), FATSector(FATSector)
	{
		// �����ļ���
		string name(start, 8);
		for (char i : name) {
			if (i != ' ') {
				this->name += i;
			}
		}
		// �����ļ���չ��
		string ext(start + 8, 3);
		for (char i : ext) {
			if (i != ' ') {
				this->ext += i;
			}
		}
		// �����Ƿ����ļ���
		isDir = getIntValue(start, 0xB, 1) == 0x10;
		// �������һ��д��ʱ��
		lastTime = getIntValue(start, 0x16, 2);
		// �������һ��д������
		lastDate = getIntValue(start, 0x18, 2);
		// �����ļ���С
		size = getIntValue(start, 0x1C, 4);
		// ���������Ӧ�Ŀ�ʼ����
		startClusterNum = getIntValue(start, 0x1A, 2);
		// ���뵱ǰ������ָ��
		contentPointer = dataSectors + (startClusterNum - 2) * 512;
		// �����ǲ�����������籾���ļ���.���ϼ��ļ���..
		isSpecial = start[0] == '.';
	}

	// ����ȫ��
	string getFullName() {
		return isDir ? name: name + '.' + ext;
	}

	// �����Ƿ����ļ���
	bool isFolder() override {
		return isDir;
	}

	// �������ֻ�����ļ���ʱ�Ž��в����������ļ����򷵻ؿ�
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
	// ��ȡ����
	string readContent()
	{
		string fileContent;
		unsigned clusterNo = startClusterNum;
		// ���غŴ��ڵ���0xFF8��ʾ�Ѿ��ﵽ���һ��
		while (clusterNo < 0xFF8) {
			if (clusterNo >= 0xFF0) {
				throw "encountered bad cluster";
			}
			// ��ǰ��������ָ��
			const char* contentPointer = dataSectors + (clusterNo - 2) * 512;
			fileContent += string(contentPointer, 512);
			// �ҵ���Ӧ����һ���غ�
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
	// ��ȡ֮���FAT���õ���һ�أ�һ���غ�1.5 �ֽ�
	unsigned nextClusterNo(unsigned current) {
		bool isOdd = current % 2 == 1;
		unsigned bytes = getIntValue(FATSector, current / 2 * 3, 3); 
		if (isOdd) {
			return bytes >> (3 * 4); // ��ø�1.5 bytes
		}
		else {
			return bytes & 0x000FFF; // ��Ǹ�1.5 bytes
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
	// ����дΪTrue
	bool isFolder() override {
		return true;
	}

	// �������е�����
	vector<File*> getSubItems() override {
		vector<File*> rootItems;
		// �õ���Ŀ¼����ָ��
		char* rootPointer = RootDirSectors();
		for (int i = 0;; i += 0x20) {
			if (rootPointer[i] != 0) {
				// �ҵ����ֿ�ʼ�ĵط���ÿһ��Ŀ¼��Ĵ�СΪ0x20�ֽڣ����Լ�0x20
				rootItems.push_back(
					new File(rootPointer + i, DataSectors(), FATSector()));
			}
			else {
				break;
			}
		}
		return rootItems;
	}
	// FAT���ȡ
	char *FATSector()
	{
		// boot����ռ�ݵ�������
		const int ResvdSecCnt = getIntValue(content, 14, 2);
		return content + (ResvdSecCnt * 512); // sector 1
	}

	// ��Ŀ¼����ȡ
	char *RootDirSectors() {
		// boot����ռ�ݵ�������
		const int ResvdSecCnt = getIntValue(content, 14, 2);
		// �ܹ���FAT�ı������
		const int NumFATs = getIntValue(content, 16, 1);
		// һ��FAT��ռλ��С
		const int FATSz16 = getIntValue(content, 22, 2);
		// 9 Sections for FAT1��9 Sections for FAT2
		return content + ((ResvdSecCnt + NumFATs * FATSz16) * 512);
	}

	// ��������ȡ
	char *DataSectors() {
		// ÿ�����ֽ�����Bytes/Sector��
		const int BytePerSec = getIntValue(content, 11, 2);
		// boot����ռ�ݵ�������
		const int ResvdSecCnt = getIntValue(content, 14, 2);
		// �ܹ���FAT�ı������
		const int NumFATs = getIntValue(content, 16, 1);
		// ��Ŀ¼���ļ������
		const int BPB_RootEntCnt = getIntValue(content, 17, 2);
		// һ��FAT��ռλ��С
		const int FATSz16 = getIntValue(content, 22, 2);
		// ��Ŀ¼��������������Ŀ¼����Ŀ¼����ɣ�һ��Ŀ¼���СΪ32�ֽ�
		const int RootDirSectors = ((BPB_RootEntCnt * 32) + (BytePerSec - 1)) / BytePerSec;
		return content + ((ResvdSecCnt + NumFATs * FATSz16 + RootDirSectors) * 512);
	}

	~Image() {
		delete content;
	}

};







//������������ݽṹ���������ˣ��ý���ָ��Ľ����ʹ����ˡ�
//�������õ��ĺ����ͱ���
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





//�����Ǹ�ʽ�������һ������
// ���ļ���ʱ������ļ�����Ŀ¼���������֡�
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

// ���ļ��е���ʽ���,�������һ���µ��ļ��У���Ҫ�����Ŀ¼����
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
����Separator���з�str�������������·���ķָ
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
commandΪ�����һ������
�з������������
vector<string>Ϊ���ص��зֺ������
*/
//�������õķ���������ȡ�����е����е��ʣ������-��ͷ���ܺ�-lͨ��ľͼ�Ϊ-l
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
	���Ҷ�Ӧ��Item����Root��ʼ������parents������Ŀ¼·���н��в��ң�now ָ�������ڲ�ѯ����Ŀ¼�ĵڼ�����
*/
Item* findItem(Item* root, vector<string>& parents, int now) {
	if (root == nullptr) {
		throw "Sorry, I cannot find this item";
	}
	//�����ѯ����Ŀ¼�����һ�������ظ����ɡ�
	if (now == parents.size()) {
		return root;
	}
	else {
		// ���й����������������Ŀ¼����������������������һ����������Ϊ��������Ŀ¼����һ����
		for (File* item : root->getSubItems()) {
			if (item->getFullName() == parents[now]) {
				return findItem(item, parents, now + 1);
			}
		}
		throw "Sorry, I cannot find this item";
	}
}



//������λ������©��/(ע����ʵ��ͷ��\�ǲ�����©�ģ����������Ч��ַ�ˡ�����β��\�ǿ�����©�ġ�
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
ls �����
*/
void lsOutput(Item* root, string parent) {
	// �������·������ð�ţ����С�
	print(parent.c_str());
	print(":\n");
	// ������е�����
	//���ﲻ֪������������ļ�����Ŀ¼�������һ��auto������һ����̬��
	auto subItems = root->getSubItems();
	for (auto item : subItems) {
		//��̬������������Ե��ļ�����
		item->printItem();
		//Ҫ��ĸ�ʽ
		print("  ");
	}
	print("\n");
	//�������Ŀ¼���ٵݹ������Ŀ¼�������Ϣ��
	for (auto item : subItems) {
		if (item->isDir && !item->isSpecial) {
			lsOutput(item, parent + item->name + '/');
		}
	}
}

/*
ls ��-lָ��
*/
void ls(string path){
	//�Ƚ��ַ�����λ������©��/������ȫ��
	path = formalizePath(path);
	//��ȡ������Ŀ¼��
	vector<string> parents = split(path, '/');
	//��ָ��ָ��Ҫ�ҵ���һ��item�������root����image��Ŀ¼��root������ò���Ҫ�ӵ�һ����ʼ�ҡ�
	Item *root = findItem(&image, parents, 0);
	//�����һ��Ŀ¼���Ͱ�Ŀ¼�������
	if (root->isFolder()) {
		lsOutput(root, path);
	}
	//�����һ���ļ���ֱ���ӳ�������Ϣ���ɡ�
	else {
		throw "Sorry, it is a file.";
	}
}
void count_print_folder_detail(Item* root,string path,int type) {
	//typeΪ1��ʾ��ӡ����ÿ����һ�еĸ�Ŀ¼����Ϣ��Ҫ����·�����Ƶ�
	//typeΪ2��ʾ��ӡ���Ǻ�ɫ��Ŀ¼���������������ֽ�����Ϣ��
	auto subItems = root->getSubItems();
	int fileCount = 0, FolderCount = 0;
	for (auto item : subItems) {
		// . & .. ����¼
		//����Ҫͳ�����е��ļ������ļ�������
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
		//�����.��..���������Ȼ���о����ˡ�
		if(thisroot->isDir && !thisroot->isSpecial)print_As_Ordered(FolderCount, fileCount);
		else
		{
			print("\n");
		}
	}
	return;
}




void ls_lOuptut(Item* root, string parent) {
	//�����parentָ����������ȫ·����
	// step1:ͳ�����е��ļ������ļ������������һ�л�����Ϣ��
	auto subItems = root->getSubItems();
	count_print_folder_detail(root, parent, 1);


	//step2:����������ļ��к����ļ��Ļ�����Ϣ������һ�У�
	// ѭ�������ǰĿ¼����
	for (auto item : subItems) {
		// �ļ�����ļ������ļ���С���ɣ����У��ļ���С��ls_l��������ģ�����Ҫ����ls_l��
		if (!item->isFolder()) {
			ls_l(parent + item->getFullName());
			print("\n");
		}
		else {
			// �������Ŀ���ļ��У���Ҫ���ֱ����Ŀ¼��ֱ�����ļ��������Ҫ��itemΪ������ͳ�Ƹ���Ŀ¼��ֱ����Ŀ¼����ֱ�����ļ�����
			count_print_folder_detail(item, parent, 2);
		}
	}
	print("\n");
	// step3:�ݹ�������ļ������ļ��е����
	for (auto item : subItems) {
		if (item->isDir && !item->isSpecial) {
			ls_lOuptut(item, parent + item->name + '/');
		}
	}
}

/*
ls -lָ��
*/
void ls_l(string path) {

	vector<string> parents = split(path, '/');
	Item *root = findItem(&image, parents, 0);
//������ļ�������ʽ����ļ������ļ����Ⱦͽ����ˡ�
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
//������ļ��У���Ҫ��ʼһ��һ�������ˡ�
	ls_lOuptut(root, path);
}

/*
cat����
*/
void cat(string path) {
	//�ȰѸ�����Ŀ¼����ȡ������
	vector<string> parents = split(path, '/');
	Item *root = findItem(&image, parents, 0);
	//
	if (root->isFolder()) {
		throw "This is a folder not a file.";
	}
	print(((File*)root)->readContent().c_str());
}

/*
	�����Ǵ���ÿһ�е����������Ӧ�ķ���
*/
void input() {

	string input;
	getline(cin, input);//������һ�ζ���һ��ָ�ֱ�Ӻ�������Ŀո�
	vector<string> commandList = commandSplit(input);
	//������һ��ָ��ֻ�����˴�����Ϣ���������ָ��Ҫ���ϻ��з���
	print("\n");
//�������Ϊ�գ���ʲôҲ������
	if (commandList.size() == 0) {
		return;
	}
	// ls �����
	if (commandList[0] == "ls") {
		// û�и������
		if (commandList.size() == 1) {
			ls("/");
		}
		else {
			//���￪ʼ�ж�lsָ���Ƿ�Ϸ��ˡ�
			int target = 0;
			//target��ʾҪ�����Ŀ���ַ���ļ�����Ŀ��
			string dict = "";
			int hasL = 0;
			//hasL��ע�������еĲ������Ƿ���-l��
			for (int i = 1; i < commandList.size(); i++) {
				if (commandList[i] == "-l")hasL = 1;
				//�������-l�Ļ���������Ŀ���ַ����dict��¼Ŀ���ַ����target��¼Ŀ���ַ����Ŀ��
				else if (regex_match(commandList[i], regex("/[A-Z0-9./]+")) || commandList[i] == "/") {
					target++;
					dict = commandList[i];
				}
				else throw "Sorry, you entered the wrong param in ls command.";
			}
			// ����ֻ�ܴ�ӡһ��
			if (target >= 2)throw "Sorry,there are too many files to ls";
			// ���û��-l����������ls address
			if (hasL == 0) {
				ls(commandList[1]);
			}
			//�����-l��������Ŀ���ַ��Ŀ���ַ��ʼ��û��Ŀ���ַ�Ӹ�Ŀ¼��ʼ��
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
	// cat ����
	else if (commandList[0] == "cat") {
		//���û�в���
		if (commandList.size() == 1) {
			throw "Sorry, the input isnot a path of file";
		}
		//����������ࡣ
		else if (commandList.size() > 2) {
			throw "Sorry,there are too many files to cat";
		}
		else {
			cat(commandList[1]);
		}
	}
	// exit����
	else if (commandList[0] == "exit"&&commandList.size() == 1) {
		throw 0;
	}
	// δ֪����
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
		//���е��쳣���������ˣ�ֻ�з�����һ�����ֲŻ��˳���
		catch (int exitCode) {
			// ûʲô����ͷ���0�����˳�
			return exitCode;
		}
		print("\n");
	}
}