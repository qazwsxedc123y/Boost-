#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <mutex>
#include "util.hpp"
#include "log.hpp"

namespace ns_index
{
    struct DocInfo // 正排索引结构体  表示文档信息
    {
        std::string title;    // 文档标题
        std::string content;  // 文档内容
        std::string url;      // 文档的url
        uint64_t doc_id;      //文档的ID
    };

    // 一个【关键字】可能出现在 无数个 【文档】中 ，我们需要根据权重判断 文档的重要顺序
    // 该单个结构体对象标识只对应一个倒排索引
    struct InvertedElem // 倒排对应的文档结构信息
    {
        std::string word;     // 关键字（通过关键字可以找到对应的ID）
        uint64_t doc_id;      // 文档ID
        int weight;           // 权重（用于排序）
    };

    // 倒排拉链  -- 一个关键字 可能存在于多个文档中，所以一个关键字对应了一组文档
    typedef std::vector<InvertedElem> InvertedList;

    class Index
    {
    private:
        // 正排索引采用数组的形式，数组的下标就是文档的ID
        std::vector<DocInfo> forward_index;

        // 一个关键字和一组（或者一个）InvertedElem对应，关键字和倒排拉链的映射关系
        std::unordered_map<std::string, InvertedList> inverted_index;
    
    public:

        ~Index()
        {}
    // 将 Index 转变成单例模式
    private:
        Index()
        {}
        Index(const Index&) = delete;
        Index& operator= (const Index&) = delete;
        static Index* instance;
        static std::mutex mtx;
    public:
        //获取index单例
        static Index* GetInstance()
        {
            if (instance == nullptr)
            {
                mtx.lock();
                if (instance == nullptr)
                {
                    instance = new Index();
                }
                mtx.unlock();//解锁 
            }
            return instance;
        }
    public:
        // 根据doc_id找到正排索引对应doc_id的文档信息
        DocInfo* GetForwardIndex(uint64_t doc_id)
        {
            // 如果这个doc_id已经大于正排索引的元素个数，则索引失败
            if(doc_id > forward_index.size())
            {
                std::cerr << "doc_id out range, error!" << std::endl;
                return nullptr;
            }
            // 否则返回相对应下标的文档信息
            return &forward_index[doc_id];
        }
        
        // 根据倒排索引的关键字word，获得倒排拉链
        InvertedList* GetInvertedList(const std::string &word)
        {
            // word关键字不是在 unordered_map 中，直接去里面找对应的倒排拉链即可
            auto iter = inverted_index.find(word);
            // 不在则返回空
            if(iter == inverted_index.end())
            {
                std::cerr << "have no InvertedList" << std::endl;
                return nullptr;
            }
            // 否则返回 unordered_map 中的第二个元素--- 倒排拉链
            return &(iter->second);
        }
        
        // 根据去标签，格式化后的文档，构建正排和倒排索引                                                                                                              
        // 将数据源的路径：data/raw_html/raw.txt传给input即可，这个函数用来构建索引
        bool BuildIndex(const std::string &input)
        {
            // 先打开文件
            std::ifstream in(input, std::ios::in | std::ios::binary);
            if(!in.is_open())
            {
                std::cerr << input << "open err" << std::endl;
                return false;
            }

            int count = 0;
            std::string line;
            while(getline(in,line))
            {
                // 根据读取的一个文档信息构建索引
                
                // 先构建正排索引
                DocInfo* doc = BuildForwardIndex(line);//构建正排索引
                if(doc == nullptr)
                {
                    std::cerr << "build " << line << " error" << std::endl;
                    continue;
                }
                // 再构建倒排索引
                BuildInvertedIndex(*doc);//有了正排索引才能构建倒排索引

                count++;    
                if(count % 50 == 0)    
                {    
                    std::cout << "当前已经建立的索引文档：" << count << "个" << std::endl;     
                }
            } 
            return true;
        }

    private:
        // 构建正排索引 将拿到的一行html文件传输进来（其也就是单个文档信息），进行解析
        // 大致：文档信息填充一个DocInfo结构体变量中，然后将其插入正排索引的vector中即可
        DocInfo* BuildForwardIndex(const std::string &line)
        {
            // 先进行解析line -- 进行字符串切分
            // 切分为三部分 string(title, content, url)
            std::vector<std::string> results; // 将切分后结果存放到此
            std::string sep = "\3"; //行内分隔符
            ns_util::StringUtil::Splist(line, &results, sep);//字符串切分 
            if(results.size() != 3)
            {
                std::cerr << "build forward index --> Splist" << std::endl;
                return nullptr;
            }
            // 切分完后将其填充到DocInfo类型的变量中
            DocInfo doc;
            doc.title = results[0];
            doc.content = results[1];
            doc.url = results[2];
            doc.doc_id = forward_index.size();

            // 3.插入到正排索引的vector                                         
            forward_index.push_back(std::move(doc)); //使用move可以减少拷贝带来的效率降低
            return &forward_index.back(); 
        }

        // 构建倒排索引
        bool BuildInvertedIndex(const DocInfo &doc)
        {
            // 权重由词频统计代替
            // 构建词频统计结构体
            struct word_cnt
            {
                int title_cnt;
                int content_cnt;
                word_cnt():title_cnt(0), content_cnt(0){}
            };

            std::unordered_map<std::string, word_cnt> word_map; // 用来暂存词频的映射表

            // 对标题进行分词
            std::vector<std::string> title_words;
            ns_util::JiebaUtil::CutString(doc.title, &title_words);

            // 对标题的词频统计
            for(auto s : title_words)
            {
                boost::to_lower(s);
                word_map[s].title_cnt++;
            }

            // 对内容进行分词
            std::vector<std::string> content_words;
            ns_util::JiebaUtil::CutString(doc.content, &content_words);
            
            // 对内容的词频统计
            for(auto s : content_words)
            {
                boost::to_lower(s);
                word_map[s].content_cnt++;
            }

#define X 10    
#define Y 1 
            // 最后构建倒排
            for(auto &word_pair : word_map)
            {
                InvertedElem item;
                item.word = word_pair.first;
                item.doc_id = doc.doc_id;
                item.weight = word_pair.second.title_cnt * X + word_pair.second.content_cnt * Y;
                InvertedList& inverted_list = inverted_index[word_pair.first]; 
                inverted_list.push_back(std::move(item));
            }

            return true;
        }

    };
    Index* Index::instance = nullptr;
    std::mutex Index::mtx;
}