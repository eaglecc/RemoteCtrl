﻿// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
    : CDialog(IDD_DLG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
    ON_WM_TIMER()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDBLCLK()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序

// 将用户坐标转换为服务器屏幕坐标
CPoint CWatchDialog::UserPoint2ScreenPoint(CPoint& point)
{
    CRect clientRect;
    ScreenToClient(&point); // 全局坐标转换为客户区坐标
    // 本地坐标转换为远程坐标
    m_picture.GetWindowRect(clientRect); // 获取控件的矩形区域
    return CPoint(point.x * 2560 / clientRect.Width(), point.y * 1440 / clientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    SetTimer(0, 45, NULL);

    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (nIDEvent == 0) {
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        if (pParent->isFull()) {
            CRect rect;
            m_picture.GetWindowRect(rect);
            //pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
            pParent->GetImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY); // 缩放
            m_picture.InvalidateRect(NULL);
            pParent->GetImage().Destroy();
            pParent->SetImageStatus();
        }
    }
    CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    //坐标转换
    CPoint remote = UserPoint2ScreenPoint(point);
    //封装
    MOUSEEV event;
    event.nAction = 2;
    event.nButton = 0;
    event.ptXY = remote;
    //发送
    CClientSocket* pClient = CClientSocket::getInstance();
    CPacket pack(5, (BYTE*)&event, sizeof(event));
    pClient->Send(pack);
    CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
    CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
    ////坐标转换
    //CPoint remote = UserPoint2ScreenPoint(point);
    ////封装
    //MOUSEEV event;
    //event.nAction = 4;
    //event.nButton = 0;
    //event.ptXY = remote;
    ////发送
    //CClientSocket* pClient = CClientSocket::getInstance();
    //CPacket pack(5, (BYTE*)&event, sizeof(event));
    //pClient->Send(pack);
    CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
    //坐标转换
    CPoint remote = UserPoint2ScreenPoint(point);
    //封装
    MOUSEEV event;
    event.nAction = 2;
    event.nButton = 2;
    event.ptXY = remote;
    //发送
    CClientSocket* pClient = CClientSocket::getInstance();
    CPacket pack(5, (BYTE*)&event, sizeof(event));
    pClient->Send(pack);
    CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
    //坐标转换
    CPoint remote = UserPoint2ScreenPoint(point);
    //封装
    MOUSEEV event;
    event.nAction = 3;
    event.nButton = 2;
    event.ptXY = remote;
    //发送
    CClientSocket* pClient = CClientSocket::getInstance();
    CPacket pack(5, (BYTE*)&event, sizeof(event));
    pClient->Send(pack);
    CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
    //坐标转换
    CPoint remote = UserPoint2ScreenPoint(point);
    //封装
    MOUSEEV event;
    event.nAction = 4;
    event.nButton = 2;
    event.ptXY = remote;
    //发送
    CClientSocket* pClient = CClientSocket::getInstance();
    CPacket pack(5, (BYTE*)&event, sizeof(event));
    pClient->Send(pack);
    CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
    //坐标转换
    CPoint remote = UserPoint2ScreenPoint(point);
    //封装
    MOUSEEV event;
    event.nAction = 1;
    event.nButton = 0;
    event.ptXY = remote;
    //发送
    CClientSocket* pClient = CClientSocket::getInstance();
    CPacket pack(5, (BYTE*)&event, sizeof(event));
    pClient->Send(pack);
    CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
    CPoint point;
    GetCursorPos(&point);

    //坐标转换
    CPoint remote = UserPoint2ScreenPoint(point);
    //封装
    MOUSEEV event;
    event.nAction = 0;
    event.nButton = 0;
    event.ptXY = remote;
    //发送
    CClientSocket* pClient = CClientSocket::getInstance();
    CPacket pack(5, (BYTE*)&event, sizeof(event));
    pClient->Send(pack);
}
