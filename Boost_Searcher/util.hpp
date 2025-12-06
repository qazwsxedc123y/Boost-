#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include "cppjieba/Jieba.hpp"

namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string &file_path, std::string *out)
        {
            // 读取 file_path（一个.html文件） 中的内容  -- 打开文件
            std::ifstream in(file_path, std::ios::in);
            if(!in.is_open())
            {
                std::cerr << "open file" << file_path << "err" << std::endl;
                return false;
            }
            //读取文件内容
            std::string line;
             while(std::getline(in, line))
            {
                *out += line;
            }
            in.close();
            return true;
        }
        static bool ParseTitle(const std::string &file, std::string *title)
        {
            std::size_t begin = file.find("<title>");
            if(begin == std::string::npos)
            {
                return false;
            }

            std::size_t end = file.find("</title>");
            if(end == std::string::npos)
            {
                return false;
            }

            begin += std::string("<title>").size();
            if(begin > end)
            {
                return false;
            }
            *title = file.substr(begin, end - begin);
            return true;
        }
        
        static bool ParseContent(const std::string &file, std::string *content)
        {
            // 去标签，基于一个简易的状态机
            enum status
            {
                LABLE,   // 标签
                CONTENT  // 内容
            };
            enum status s = LABLE;  // 刚开始肯定会碰到 "<" 默认状态为 LABLE
            for(char c : file)
            {
                // 检测状态
                switch(s)
                {
                    case LABLE:
                        if(c == '>') s = CONTENT;
                        break;
                    case CONTENT:
                        if(c == '<') s = LABLE;
                        else
                        {
                            // 我们不想保留原始文件中的\n，因为我们想用\n作为html解析之后的文本的分隔符
                            if(c == '\n') c = ' ';
                            content->push_back(c);
                        }
                        break;
                    default:
                        break;
                }
            }
            return true;
        }
        static bool Parseurl(const std::string &file_path, std::string* url)
        {
            const std::string src_path = "data/input";          // 数据源的路径
            std::string url_head = "https://www.boost.org/doc/libs/1_89_0/doc/html";
            std::string url_tail = file_path.substr(src_path.size());//将data/input截取掉    
            *url = url_head + url_tail;//拼接
            return true;
        }
    };


    class StringUtil
    {
    public:
        //切分字符串
        static void Splist(const std::string &target, std::vector<std::string> *out, const std::string &sep)
        {
            //boost库中的split函数
            boost::split(*out, target, boost::is_any_of(sep), boost::token_compress_on);
            // 第一个参数：表示你要将切分的字符串放到哪里
            // 第二个参数：表示你要切分的字符串
            // 第三个参数：表示分割符是什么，不管是多个还是一个
            // 第四个参数：它是默认可以不传，即切分的时候不压缩，不压缩就是保留空格
            // 如：字符串为aaaa\3\3bbbb\3\3cccc\3\3d
            // 如果不传第四个参数 结果为aaaa  bbbb  cccc  d
            // 如果传第四个参数为boost::token_compress_on 结果为aaaabbbbccccd
            // 如果传第四个参数为boost::token_compress_off 结果为aaaa  bbbb  cccc  d
        }
    };


const char* const DICT_PATH = "./dict/jieba.dict.utf8";
const char* const HMM_PATH = "./dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "./dict/user.dict.utf8";
const char* const IDF_PATH = "./dict/idf.utf8";
const char* const STOP_WORD_PATH = "./dict/stop_words.utf8";

    class JiebaUtil
    {
    private:
        static cppjieba::Jieba jieba; //定义静态的成员变量（需要在类外初始化）
    public:    
        static void CutString(const std::string &src, std::vector<std::string> *out)    
        {   
            //调用CutForSearch函数，第一个参数就是你要对谁进行分词，第二个参数就是分词后的结果存放到哪里
            jieba.CutForSearch(src, *out);    
        }   
    };
    //类外初始化，就是将上面的路径传进去，具体和它的构造函数是相关的，具体可以去看一下源代码
    cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);
}