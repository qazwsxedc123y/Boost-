#include <iostream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "util.hpp"

const std::string src_path = "data/input";          // 数据源的路径
const std::string output = "data/raw_html/raw.txt"; // 清理后干净文档的路径

// DocInfo --- 文件信息结构体
typedef struct DocInfo
{
    std::string title;
    std::string content;
    std::string url;
}DocInfo_t;

// const & ---> 输入
// * ---> 输出
// & ---> 输入输出

//把每个html文件名带路径，保存到files_list中
bool EnumFile(const std::string &src_path, std::vector<std::string> *files_list);
 
//按照files_list读取每个文件的内容，并进行解析
bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results);
 
//把解析完毕的各个文件的内容写入到output
bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output);


int main()
{
    std::vector<std::string> files_list; // 将所有的 html文件名保存在 files_list 中
    
    // 第一步先递归式的读取各个目录，把每个html文件名带路径，保存到files_list中，方便后期进行一个一个的文件读取
    if(!EnumFile(src_path, &files_list))
    {
        std::cerr << "enum file name error! " << std::endl;
        return 1;
    }

    // 从 src_path 这个路径中提取 html文件，把提取出的文件以string的类型存放在files_list中
    

    // 第二步：按照files_list读取的文件中读取每个.html的内容，进行解析，存放在DocInfo类型的results中

    std::vector<DocInfo_t> results;
    if(!ParseHtml(files_list, &results))
    {
        std::cerr << "parse html error! " << std::endl;
        return 2;
    }
    

    // 第三步：把解析完毕的各个文件的内容写入到output，按照 \3 作为每个文档的分隔符
    // 将 results 解析好的内容，直接放入 output 路径下
    if(!SaveHtml(results, output))//SaveHtml--保存html
    {
        std::cerr << "save html error! " << std::endl;
        return 3;
    }

    return 0;
}


bool EnumFile(const std::string &src_path, std::vector<std::string> *files_list)
{
    // 引入命名空间
    namespace  fs = boost::filesystem;
    fs::path root_path(src_path); // 定义一个path对象，枚举文件就从这个路径下开始
    if(!fs::exists(root_path))
    {
        std::cerr << src_path << "not exists" << std::endl;
        return false;
    }

    // 对文件进行递归遍历
    fs::recursive_directory_iterator end; // 定义了一个迭代器，用于判断递归结束。
    for(fs::recursive_directory_iterator iter(root_path); iter != end; iter++)
    {
        // 不是常规的文件，如果指定路径是目录或图片直接跳过
        if(!fs::is_regular_file(*iter))
        {
            continue;
        }

        // 如果满足了是普通文件，还需满足是.html结尾的
        // 如果不满足也是需要跳过的
        // ---通过iter这个迭代器（理解为指针）的一个path方法（提取出这个路径）
        // ---然后通过extension()函数获取到路径的后缀

        // 但不是指定文件.html
        if(iter->path().extension() != ".html")
        {
            continue;
        }

        // std::cout << "debug: " << iter->path().string() << std::endl; // 测试代码
        // break;

        files_list->push_back(iter->path().string()); // 将所有带路径的html保存在files_list中，方便后续进行文本分析
    }

    return true;
}

// for debug
void ShowDoc(const DocInfo_t& doc)
{
    std::cout << "title: " << doc.title << std::endl;
    std::cout << "content: " << doc.content << std::endl;
    std::cout << "url: " << doc.url << std::endl;
}

bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results)
{
    //bool flag = true;
    for(const std::string &file : files_list)
    {
        // 1.读取文件，Read()
        std::string result;
        if(!ns_util::FileUtil::ReadFile(file, &result))
        {
            continue;
        }
        // 2.解析指定的文件，提取title
        DocInfo_t doc;
        if(!ns_util::FileUtil::ParseTitle(result, &doc.title))
        {
            continue;
        }
        // 3.解析指定的文件，提取content
        if(!ns_util::FileUtil::ParseContent(result, &doc.content))
        {
            continue;
        }
        // 4.解析指定的文件路径，构建url
        if(!ns_util::FileUtil::Parseurl(file, &doc.url))
        {
            continue;
        }

        // dedug
        // if(flag)
        // {        
        //     ShowDoc(doc);
        //     flag = false;
        // }


        // 到这里，一定是完成了解析任务，当前文档的相关结果都保存在了doc里面
        results->push_back(std::move(doc)); // 本质会发生拷贝，效率肯能会比较低，这里我们使用move后的左值变成了右值，去调用push_back的右值引用版本
    }
    
    return true;
}

bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output)
{
    // 保存格式：title\3content\3url \n 
    #define SEP '\3'//分割符---区分标题、内容和网址
    
    // 打开文件，在里面进行写入
    // 按照二进制的方式进行写入 -- 你写的是什么文档就保存什么
    std::ofstream out(output, std::ios::out | std::ios::binary);
    if(!out.is_open())
    {
        std::cerr << "open " << output << " failed!" << std::endl;
        return false;
    }

    for(auto &item : results)
    {
        std::string out_string;
        out_string += item.title; // 标题
        out_string += SEP; // 分隔符
        out_string += item.content; // 内容
        out_string += SEP; // 分隔符
        out_string += item.url; // 网址
        out_string += '\n'; // 用于区分不同文件

        // 将字符串内容写入文件中
        out.write(out_string.c_str(), out_string.size());
    }
    
    out.close();
    return true;
}