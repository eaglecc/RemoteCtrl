#include "pch.h"
#include "CCommand.h"

CCommand::CCommand(): threadId(0)
{
    struct {
        int nCmd;
        CMDFUNC func;
    }data[] = {
        { 1, &CCommand::MakeDriverInfo },
        { 2, &CCommand::MakeDirectoryInfo },
        { 3, &CCommand::RunFile },
        { 4, &CCommand::downloadFile },
        { 5, &CCommand::MouseEvent },
        { 6, &CCommand::SendScreen },
        { 7, &CCommand::LockMachine },
        { 8, &CCommand::UnmLockMachine },
        { 9, &CCommand::DeleteLocalFile },
        { 2000, &CCommand::TestConnect},
                { -1, NULL }
    };

    for (size_t i = 0; data[i].nCmd != -1; i++)
    {
        m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
    }
}

CCommand::~CCommand()
{
}

int CCommand::ExcuteCommand(int nCmd)
{
    std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
    if (it == m_mapFunction.end())
    {
        return -1;
    }
    return (this->*it->second)();
}
