﻿
// RemoteClientDlg.h: 头文件
//

#pragma once

#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1) // 发送数据包消息

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
    // 构造
public:
    CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
public:
    bool isFull() const {
        return m_isFull;
    }
    CImage& GetImage() {
        return m_image;
    }
    void SetImageStatus(bool isFull = false) {
        this->m_isFull = isFull;
    }
private:
    CImage m_image; // 缓存
    bool m_isFull; // 缓存是否有数据
    bool m_isClosed; // 监视窗口是否关闭

    static void threadEntryForWatchData(void* arg); // 静态函数中不能使用this指针
    void threadWathData();
    static void threadEntryForDownFile(void* arg);
    void LoadFileInfo();
    void LoadFileCurrentInfo();
    // 1. 查看磁盘分区 2. 查看指定目录的文件列表 3. 打开文件 4. 下载文件
    // 返回值是控制命令号
    int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);
    CString GetPath(HTREEITEM hTree);
    void DeleteTreeChilrenItem(HTREEITEM hTree);
    void DownLoadFile();

    // 实现
protected:
    HICON m_hIcon;
    CStatusDlg m_statusDlg;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedBtnTest();
    DWORD m_server_address;
    CString m_nPort;
    afx_msg void OnBnClickedButtonFileinfo();
    CTreeCtrl m_Tree;
    afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
    // 显示文件
    CListCtrl m_List;
    afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDownloadFile();
    afx_msg void OnDeleteFile();
    afx_msg void OnRunFile();
    afx_msg LRESULT onSendPacket(WPARAM wParam, LPARAM lParam);
    afx_msg void OnBnClickedBtnStartWatch();
};
