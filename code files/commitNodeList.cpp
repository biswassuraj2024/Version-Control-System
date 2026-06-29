#include <iostream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <ctime>
#include <vector>
#include <memory>
using namespace std;

// ================  Global Functions  ==================
string gen_random(const int len)
{
    srand((unsigned)time(NULL) * getpid());
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
    {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

string get_time()
{
    time_t t = std::time(0); // get time now
    tm *now = std::localtime(&t);
    string dateTime = to_string(now->tm_year + 1900) + "/" +
                      to_string(now->tm_mon + 1) + "/" +
                      to_string(now->tm_mday) + " " +
                      to_string(now->tm_hour) + ":" +
                      to_string(now->tm_min) + "\n";

    return dateTime;
}
// =========================================================

class commitNode
{
private:
    string commitID;
    string commitMsg;
    string nextCommitID;
    unique_ptr<commitNode> next;

    void createCommitNode()
    {
        // create commit dir, create commitInfo.txt, copy files
        filesystem::create_directory(filesystem::current_path() / ".git" / "commits" / commitID);
        auto path = filesystem::current_path() / ".git" / "commits" / commitID / "commitInfo.txt";
        ofstream write(path.string());
        write << "1." + commitID + "\n" +
                     "2." + commitMsg + "\n" +
                     "3." + get_time() + "\n";

        auto STAGING_AREA_PATH = filesystem::path(filesystem::current_path() / ".git" / "staging_area");
        const auto copyOptions = filesystem::copy_options::update_existing | filesystem::copy_options::recursive;
        if (filesystem::exists(STAGING_AREA_PATH))
        {
            filesystem::copy(STAGING_AREA_PATH, filesystem::current_path() / ".git" / "commits" / commitID / "Data", copyOptions);
        }
    }

public:
    commitNode()
    {
        this->next = nullptr;
    }
    commitNode(string commitID, string commitMsg)
    {
        this->commitID = commitID;
        this->commitMsg = commitMsg;
        this->next = nullptr;
        createCommitNode();
    }
    commitNode(string commitId)
    {
        // msg is not populated.
        this->commitID = commitId;
        checkNextCommitId();
        this->next = nullptr;
    }

    void revertCommitNode(string fromHash)
    {
        filesystem::create_directories(filesystem::current_path() / ".git" / "commits" / getCommitID() / "Data");
        auto nextCommitPath = filesystem::current_path() / ".git" / "commits" / getCommitID() / "commitInfo.txt";
        auto copyFrom = filesystem::current_path() / ".git" / "commits" / fromHash / "Data";
        ofstream write(nextCommitPath.string());
        write << "1." + commitID + "\n" +
                     "2." + commitMsg + "\n" +
                     "3." + get_time() + "\n";
        const auto copyOptions = filesystem::copy_options::recursive;
        if (filesystem::exists(copyFrom))
        {
            filesystem::copy(copyFrom, filesystem::current_path() / ".git" / "commits" / getCommitID() / "Data", copyOptions);
        }
    }

    void readNodeInfo()
    {
        string info;
        auto path = filesystem::current_path() / ".git" / "commits" / getCommitID() / "commitInfo.txt";
        if (!filesystem::exists(path)) return;
        ifstream file(path.string());
        while (getline(file, info))
        {
            if (info.length() >= 2)
            {
                if (info[0] == '1')
                {
                    this->setCommitID(info.substr(2));
                }
                if (info[0] == '2')
                {
                    this->setCommitMsg(info.substr(2));
                }
            }
        }
    }

    string getCommitMsg()
    {
        return this->commitMsg;
    }
    void setCommitMsg(string commitMsg)
    {
        this->commitMsg = commitMsg;
    }

    void setCommitID(string commitID)
    {
        this->commitID = commitID;
    }
    string getCommitID()
    {
        return this->commitID;
    }

    void setNext(unique_ptr<commitNode> node)
    {
        this->next = move(node);
    }
    commitNode *getNext()
    {
        return next.get();
    }
    unique_ptr<commitNode> releaseNext()
    {
        return move(next);
    }

    void setNextCommitID(string nextCommitId)
    {
        this->nextCommitID = nextCommitId;
    }
    void writeNextCommitID(string nextCommitID)
    {
        setNextCommitID(nextCommitID);
        auto path = filesystem::current_path() / ".git" / "commits" / getCommitID() / "nextCommitInfo.txt";
        ofstream write(path.string());
        write << nextCommitID;
    }
    string checkNextCommitId()
    {
        filesystem::path tempPath(filesystem::current_path() / ".git" / "commits" / getCommitID() / "nextCommitInfo.txt");
        bool exists = filesystem::exists(tempPath);
        if (exists)
        {
            string info;
            ifstream file(tempPath);
            while (getline(file, info))
            {
                this->nextCommitID = info;
                break;
            }
            file.close();
            return nextCommitID;
        }
        return "";
    }
    string getNextCommitId()
    {
        return this->nextCommitID;
    }
};

class commitNodeList
{
private:
    unique_ptr<commitNode> HEAD;
    commitNode *TAIL;

    bool checkHead()
    {
        // check if HEAD commit exists
        auto tempDir = filesystem::current_path() / ".git" / "commits" / "0x1111";
        return filesystem::exists(tempDir);
    }

    void writeTailID(const string &tailID)
    {
        auto path = filesystem::current_path() / ".git" / "commits" / "TAIL.txt";
        ofstream write(path.string());
        write << tailID;
    }

    string readTailID()
    {
        filesystem::path path(filesystem::current_path() / ".git" / "commits" / "TAIL.txt");
        if (filesystem::exists(path))
        {
            string info;
            ifstream file(path);
            if (getline(file, info))
            {
                file.close();
                return info;
            }
            file.close();
        }
        return "";
    }

public:
    commitNodeList()
    {
        TAIL = nullptr;
        if (checkHead())
        {
            HEAD = make_unique<commitNode>("0x1111");
            HEAD->readNodeInfo();
            commitNode *currNode = HEAD.get();
            while (currNode != nullptr)
            {
                string nextId = currNode->checkNextCommitId();
                if (nextId.length() < 8)
                {
                    TAIL = currNode;
                    writeTailID(currNode->getCommitID());
                    break;
                }
                auto nextNode = make_unique<commitNode>(nextId);
                nextNode->readNodeInfo();
                commitNode *nextPtr = nextNode.get();
                currNode->setNext(move(nextNode));
                currNode = nextPtr;
            }
        }
    }

    ~commitNodeList()
    {
        // Iterative cleanup to prevent stack overflow on large commit graphs
        while (HEAD != nullptr)
        {
            HEAD = HEAD->releaseNext();
        }
    }

    commitNode *getHead()
    {
        return this->HEAD.get();
    }
    void setHead(unique_ptr<commitNode> newHead)
    {
        this->HEAD = move(newHead);
    }

    commitNode *getTail()
    {
        return this->TAIL;
    }
    void setTail(commitNode *newTail)
    {
        this->TAIL = newTail;
    }

    void addOnTail(string msg)
    {
        if (!checkHead())
        {
            auto newNode = make_unique<commitNode>("0x1111", msg);
            TAIL = newNode.get();
            setHead(move(newNode));
            writeTailID("0x1111");
        }
        else
        {
            string commitID = gen_random(8);
            auto newNode = make_unique<commitNode>(commitID, msg);
            commitNode *newTailPtr = newNode.get();
            if (getTail() != nullptr)
            {
                getTail()->writeNextCommitID(commitID);
                getTail()->setNext(move(newNode));
            }
            setTail(newTailPtr);
            writeTailID(commitID);
        }
    }

    void revertCommit(string commitHash)
    {
        if (!checkHead())
        {
            cout << "Nothing to Revert to " << endl;
            return;
        }
        if (commitHash == "HEAD" && getTail() != nullptr)
        {
            commitHash = getTail()->getCommitID();
        }

        commitNode *commitNodeToRevert = nullptr;
        commitNode *currNode = getHead();
        while (currNode != nullptr)
        {
            if (commitHash == currNode->getCommitID())
            {
                commitNodeToRevert = currNode;
                break;
            }
            currNode = currNode->getNext();
        }

        if (commitNodeToRevert == nullptr)
        {
            cout << "does not match any Commit Hash" << endl;
            return;
        }

        string newCommitID = gen_random(8);
        auto newNode = make_unique<commitNode>();
        newNode->setCommitID(newCommitID);
        newNode->setCommitMsg(commitNodeToRevert->getCommitMsg());
        newNode->revertCommitNode(commitNodeToRevert->getCommitID());

        commitNode *newTailPtr = newNode.get();
        if (getTail() != nullptr)
        {
            getTail()->writeNextCommitID(newCommitID);
            getTail()->setNext(move(newNode));
        }
        setTail(newTailPtr);
        writeTailID(newCommitID);
    }

    void printCommitList()
    {
        commitNode *currNode = getHead();
        while (currNode != nullptr)
        {
            cout << "Commit ID:    " << currNode->getCommitID() << endl;
            cout << "Commit Msg:   " << currNode->getCommitMsg() << endl;
            
            filesystem::path commitPath(filesystem::current_path() / ".git" / "commits" / currNode->getCommitID() / "commitInfo.txt");
            if (filesystem::exists(commitPath))
            {
                string info;
                ifstream file(commitPath.string());
                while (getline(file, info))
                {
                    if (info.length() >= 2 && info[0] == '3')
                    {
                        cout << "Date & Time:  " << info.substr(2) << endl;
                    }
                }
                file.close();
            }
            cout << "============================\n\n";

            currNode = currNode->getNext();
        }
    }

    // void printCommitStatus()
    // {
    //     // vector<string> staging_area_files;
    //     // for (const filesystem::path &file : filesystem::recursive_directory_iterator(filesystem::current_path() / ".git" / "staging_area"))
    //     // {
    //     //     staging_area_files.push_back(file.string().substr( file.string().find_last_of('/') + 1 ));
    //     // }
    //     // vector<string> latest_commit_files;
    //     // for (const filesystem::path &file : filesystem::recursive_directory_iterator(filesystem::current_path() / ".git" / getTail()))
    //     // {
    //     //     latest_commit_files.push_back(file.string().substr( file.string().find_last_of('/') + 1 ));
    //     // }

    //     // if (staging_area_files.size() == latest_commit_files.size())
    //     // {
    //     //     cout << "the branch is up-to-date";
    //     // }
    //     // else
    //     // {
    //     //     cout << "untracked files: \n";
    //     //     for (auto i = v.begin(); i != v.end(); i++)
    //     //     {
    //     //     }
    //     // }
    // }
};
