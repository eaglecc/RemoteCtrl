
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    // 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
    , m_server_address(0)
    , m_nPort(_T(""))
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
    DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
    DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
    DDX_Control(pDX, IDC_LIST_FILE, m_List);
}


BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
    ON_BN_CLICKED(IDC_BUTTON_FILEINFO, &CRemoteClientDlg::OnBnClickedButtonFileinfo)
    ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
    ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
    ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
    ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
    ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
    ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
    ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::onSendPacket)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO: 在此添加额外的初始化代码
    UpdateData();
    m_server_address = 0x7F000001;
    m_nPort = _T("9527");
    UpdateData(FALSE);

    m_statusDlg.Create(IDD_DIALOG_STATUS);
    m_statusDlg.ShowWindow(SW_HIDE);

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
    UpdateData(); //这行代码放在子线程中执行会崩溃
    CClientSocket* pClient = CClientSocket::getInstance();
    bool ret = pClient->InitServer(m_server_address, atoi((LPCTSTR)m_nPort));
    if (!ret) {
        AfxMessageBox(_T("连接服务器失败"));
        return -1;
    }
    CPacket packet(nCmd, pData, nLength);
    ret = pClient->Send(packet);
    TRACE("[客户端] 客户端发送结果:%d\r\n", ret);
    int cmd = pClient->DealCommand();
    TRACE("[客户端] 客户端收到服务端发来的命令号:%d\r\n", cmd);
    if (bAutoClose)
        pClient->CloseSocket();

    return cmd;
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
    SendCommandPacket(2000);
}


void CRemoteClientDlg::OnBnClickedButtonFileinfo()
{
    int ret = SendCommandPacket(1);
    if (ret == -1)
    {
        AfxMessageBox(_T("命令处理失败！"));
        return;
    }
    CClientSocket* pClient = CClientSocket::getInstance();
    std::string drivers = pClient->GetPacket().sData; //"C,D,E"
    m_Tree.DeleteAllItems();
    std::string dr;
    for (size_t i = 0; i < drivers.size() + 1; i++)
    {
        if (drivers[i] == ',' || i == (drivers.size()))
        {
            dr += ":";
            HTREEITEM hTmp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
            m_Tree.InsertItem("", hTmp, TVI_LAST);//目录插入一个空项
            dr.clear();
            continue;
        }
        dr += drivers[i];
    }
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
    CString strRet, strTmp;
    do {
        strTmp = m_Tree.GetItemText(hTree);
        strRet = strTmp + '\\' + strRet;
        hTree = m_Tree.GetParentItem(hTree);
    } while (hTree != NULL);
    return strRet;
}

// 删除子节点
void CRemoteClientDlg::DeleteTreeChilrenItem(HTREEITEM hTree)
{
    HTREEITEM hSub = NULL;
    do {
        hSub = m_Tree.GetChildItem(hTree);
        if (hSub != NULL) m_Tree.DeleteItem(hSub);
    } while (hSub != NULL);
}

// 下载文件线程入口函数
void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
    CRemoteClientDlg* th = (CRemoteClientDlg*)arg;
    th->DownLoadFile();
    _endthread();
}

// 下载文件
void CRemoteClientDlg::DownLoadFile()
{
    int nListSelected = m_List.GetSelectionMark();
    CString strPath = m_List.GetItemText(nListSelected, 0);// 文件名

    CFileDialog dlg(false, "*", strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
    // 参数strPath是文件保存到本地后的文件名
    if (dlg.DoModal() == IDOK) {// 成功在文件对话框中选择了保存路径
        FILE* pFile = fopen(dlg.GetPathName(), "wb+");// 打开文件
        if (pFile == NULL) {
            AfxMessageBox(_T("文件无法创建打开或无权限编辑该文件！"));
            m_statusDlg.ShowWindow(SW_HIDE);
            return;
        }

        HTREEITEM hTreeSelected = m_Tree.GetSelectedItem();
        strPath = GetPath(hTreeSelected) + strPath;// 文件路径
        TRACE("getPath:%s\r\n", strPath);

        CClientSocket* pclient = CClientSocket::getInstance();

        //int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
        // 使用消息机制发送数据包
        int ret = SendMessageA(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strPath);
        if (ret == -1) {
            AfxMessageBox(_T("命令解析失败！"));
            m_statusDlg.ShowWindow(SW_HIDE);
            fclose(pFile);
            pclient->CloseSocket();
            return;
        }

        long long nLength = *(long long*)pclient->GetPacket().sData.c_str();// 先接收包含文件长度信息的包
        if (nLength == 0) {
            AfxMessageBox(_T("无法读取文件或文件为空！"));
            m_statusDlg.ShowWindow(SW_HIDE);
            fclose(pFile);
            pclient->CloseSocket();
            return;
        }

        long long nCount = 0;
        while (nCount < nLength) {
            int ret = pclient->DealCommand();
            if (ret < 0) {
                AfxMessageBox(_T("传输文件失败！"));
                break;
            }
            fwrite(pclient->GetPacket().sData.c_str(), 1, pclient->GetPacket().sData.size(), pFile);// 写入文件
            nCount += pclient->GetPacket().sData.size();
        }

        fclose(pFile);
        pclient->CloseSocket();
    }
    m_statusDlg.ShowWindow(SW_HIDE);
    EndWaitCursor();
    AfxMessageBox(_T("下载完成"));
}


void CRemoteClientDlg::LoadFileInfo()
{
    CPoint ptMouse;//鼠标指针
    GetCursorPos(&ptMouse);//获取鼠标指针坐标
    m_Tree.ScreenToClient(&ptMouse);//转为树控件中的坐标

    //获得鼠标点击坐标对应的树节点句柄
    HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
    if (hTreeSelected == NULL)return;
    if (m_Tree.GetChildItem(hTreeSelected) == NULL)return; // 文件节点，直接返回

    //先删除孩子节点防止重复点击
    DeleteTreeChilrenItem(hTreeSelected);
    //清空文件列表
    m_List.DeleteAllItems();

    CString strPath = GetPath(hTreeSelected);
    TRACE("path=%s\r\n", strPath);
    // 发送路径信息给服务端
    int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
    if (nCmd < 0)
    {
        AfxMessageBox(_T("命令处理失败！"));
        return;
    }

    CClientSocket* pClient = CClientSocket::getInstance();
    PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().sData.c_str();

    while (pInfo->HasNext) {
        TRACE("[客户端] [CRemoteClientDlg::OnNMDblclkTreeDir] 文件名:%s，是否是文件:%d\r\n", pInfo->szFileName, pInfo->IsDirectory);
        if (pInfo->IsDirectory) {
            if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") {
                int cmd = pClient->DealCommand();
                TRACE("[客户端] [CRemoteClientDlg::OnNMDblclkTreeDir] ack:%d\r\n", cmd);
                if (cmd < 0) break;
                pInfo = (PFILEINFO)pClient->GetPacket().sData.c_str();
                continue;
            }
            HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
            m_Tree.InsertItem("", hTemp, TVI_LAST); // 给文件夹加上一个空的子结点，不加这句话后面很麻烦
        }
        else {
            // 如果是文件，就显示在列表控件中
            m_List.InsertItem(0, pInfo->szFileName);
        }
        // 处理下一个文件信息
        int cmd = pClient->DealCommand();
        TRACE("[客户端] [CRemoteClientDlg::OnNMDblclkTreeDir] ack:%d\r\n", cmd);
        if (cmd < 0) break;
        pInfo = (PFILEINFO)pClient->GetPacket().sData.c_str();
    } // 服务器端在最后发送了一个hasNext为false的包

    pClient->CloseSocket();
}

void CRemoteClientDlg::LoadFileCurrentInfo()
{
    HTREEITEM hTreeSelected = m_Tree.GetSelectedItem();
    CString strPath = GetPath(hTreeSelected);
    //清空文件列表
    m_List.DeleteAllItems();

    // 发送路径信息给服务端
    int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());
    if (nCmd < 0)
    {
        AfxMessageBox(_T("命令处理失败！"));
        return;
    }

    CClientSocket* pClient = CClientSocket::getInstance();
    PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().sData.c_str();

    while (pInfo->HasNext) {
        TRACE("[客户端] [CRemoteClientDlg::OnNMDblclkTreeDir] 文件名:%s，是否是文件:%d\r\n", pInfo->szFileName, pInfo->IsDirectory);
        if (!pInfo->IsDirectory) {
            // 如果是文件，就显示在列表控件中
            m_List.InsertItem(0, pInfo->szFileName);
        }
        // 处理下一个文件信息
        int cmd = pClient->DealCommand();
        TRACE("[客户端] [CRemoteClientDlg::OnNMDblclkTreeDir] ack:%d\r\n", cmd);
        if (cmd < 0) break;
        pInfo = (PFILEINFO)pClient->GetPacket().sData.c_str();
    } // 服务器端在最后发送了一个hasNext为false的包

    pClient->CloseSocket();
}

// 树控件双击
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    LoadFileInfo();
}

// 树控件单击
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;
    LoadFileInfo();
}

// 列表控件右键，实现文件打开下载功能
void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    *pResult = 0;
    CPoint ptMouse, ptList;
    GetCursorPos(&ptMouse);
    ptList = ptMouse;
    m_List.ScreenToClient(&ptList);
    int ListSelected = m_List.HitTest(ptList);
    if (ListSelected < 0) return;
    CMenu menu;
    menu.LoadMenu(IDR_MENU_RCLICK);
    CMenu* pSubMenu = menu.GetSubMenu(0);
    if (pSubMenu != NULL)
    {
        pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
    }
}

// 下载文件
void CRemoteClientDlg::OnDownloadFile()
{
    // 添加线程函数，让写入文件操作放在子线程中
    _beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
    BeginWaitCursor();
    m_statusDlg.m_info.SetWindowText(_T("正在下载文件，请稍后..."));
    m_statusDlg.ShowWindow(SW_SHOW);
    m_statusDlg.CenterWindow(this);
    m_statusDlg.SetActiveWindow();
    Sleep(50);
}


void CRemoteClientDlg::OnDeleteFile()
{
    HTREEITEM hSelected = m_Tree.GetSelectedItem();
    CString strPath = GetPath(hSelected);
    int nSelected = m_List.GetSelectionMark();
    CString strFile = m_List.GetItemText(nSelected, 0);
    strFile = strPath + strFile;
    int ret = SendCommandPacket(9, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
    if (ret < 0) {
        AfxMessageBox(_T("删除命令解析失败！"));
        return;
    }
    LoadFileCurrentInfo();
    AfxMessageBox(_T("删除成功！"));
}


void CRemoteClientDlg::OnRunFile()
{
    HTREEITEM hSelected = m_Tree.GetSelectedItem();
    CString strPath = GetPath(hSelected);
    int nSelected = m_List.GetSelectionMark();
    CString strFile = m_List.GetItemText(nSelected, 0);
    strFile = strPath + strFile;
    int ret = SendCommandPacket(3, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
    if (ret < 0) {
        AfxMessageBox(_T("命令解析失败！"));
        return;
    }
}

LRESULT CRemoteClientDlg::onSendPacket(WPARAM wParam, LPARAM lParam)
{
    CString strFile = (LPCTSTR)lParam;
    int ret = SendCommandPacket(wParam >> 1, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
    return ret;
}
