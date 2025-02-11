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
    m_nObjWidth = -1;
    m_nObjHeight = -1;
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
    ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
    ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序

// 将用户坐标转换为服务器屏幕坐标
CPoint CWatchDialog::UserPoint2ScreenPoint(CPoint& point, bool isScreen)
{
    CRect clientRect;
    if (isScreen) ScreenToClient(&point); // 全局坐标转换为客户区坐标
    TRACE("[客户端] x=%d, y=%d\r\n", point.x, point.y);
    // 本地坐标转换为远程坐标
    m_picture.GetWindowRect(clientRect); // 获取控件的矩形区域
    return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
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
            if (m_nObjWidth == -1) {
                m_nObjWidth = pParent->GetImage().GetWidth();
            }
            if (m_nObjHeight == -1) {
                m_nObjHeight = pParent->GetImage().GetHeight();
            }
            pParent->GetImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY); // 缩放
            m_picture.InvalidateRect(NULL);
            pParent->GetImage().Destroy();
            pParent->SetImageStatus();
        }
    }
    CDialog::OnTimer(nIDEvent);
}

// 左键双击
void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
        //坐标转换
        CPoint remote = UserPoint2ScreenPoint(point);
        //封装
        MOUSEEV event;
        event.nAction = 1;
        event.nButton = 0;
        event.ptXY = remote;
        //发送
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        pParent->SendMessageA(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
    }
    CDialog::OnLButtonDblClk(nFlags, point);
}


// 左键按下
void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
    if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
        TRACE("x=%d, y=%d\r\n", point.x, point.y);
        //坐标转换
        CPoint remote = UserPoint2ScreenPoint(point);
        TRACE("x=%d, y=%d\r\n", point.x, point.y);
        //封装
        MOUSEEV event;
        event.nAction = 2;
        event.nButton = 0;
        event.ptXY = remote;
        //发送
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        pParent->SendMessageA(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
        //CClientSocket* pClient = CClientSocket::getInstance(); // 考虑为什么不用这个发送消息
        //CPacket pack(5, (BYTE*)&event, sizeof(event));
        //pClient->Send(pack);
    }
    CDialog::OnLButtonDown(nFlags, point);
}

// 左键弹起
void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
    if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {

        //坐标转换
        CPoint remote = UserPoint2ScreenPoint(point);
        //封装
        MOUSEEV event;
        event.nAction = 3;
        event.nButton = 0;
        event.ptXY = remote;
        //发送
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        pParent->SendMessageA(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
    }
    CDialog::OnLButtonUp(nFlags, point);
}

// 右键双击
void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
    if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
        //坐标转换
        CPoint remote = UserPoint2ScreenPoint(point);
        //封装
        MOUSEEV event;
        event.nAction = 1;
        event.nButton = 1;
        event.ptXY = remote;
        //发送
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        pParent->SendMessageA(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
    }
    CDialog::OnRButtonDblClk(nFlags, point);
}

// 右键按下
void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
    if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
        //坐标转换
        CPoint remote = UserPoint2ScreenPoint(point);
        //封装
        MOUSEEV event;
        event.nAction = 2;
        event.nButton = 1;
        event.ptXY = remote;
        //发送
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        pParent->SendMessageA(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
    }
    CDialog::OnRButtonDown(nFlags, point);
}

// 右键弹起
void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
    if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
        //坐标转换
        CPoint remote = UserPoint2ScreenPoint(point);
        //封装
        MOUSEEV event;
        event.nAction = 3;
        event.nButton = 1;
        event.ptXY = remote;
        //发送
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        pParent->SendMessageA(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
    }
    CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
    if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
        //坐标转换
        CPoint remote = UserPoint2ScreenPoint(point);
        //封装
        MOUSEEV event;
        event.nAction = 0;
        event.nButton = 3;
        event.ptXY = remote;
        //发送
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        pParent->SendMessageA(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
    }
    CDialog::OnMouseMove(nFlags, point);
}

// 左键单击
void CWatchDialog::OnStnClickedWatch()
{
    if ((m_nObjHeight != -1) && (m_nObjWidth != -1)) {
        CPoint point;
        GetCursorPos(&point); // 拿到的是屏幕坐标

        //坐标转换
        CPoint remote = UserPoint2ScreenPoint(point, true);
        //封装
        MOUSEEV event;
        event.nAction = 0;
        event.nButton = 0;
        event.ptXY = remote;
        //发送
        CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获取父窗口指针
        pParent->SendMessageA(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
    }
}


void CWatchDialog::OnOK()
{
    // TODO: 在此添加专用代码和/或调用基类

    //CDialog::OnOK();
}

// 锁机
void CWatchDialog::OnBnClickedBtnLock()
{
    CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
    pParent->SendMessageA(WM_SEND_PACKET, 7 << 1 | 1);
}

// 解锁
void CWatchDialog::OnBnClickedBtnUnlock()
{
    CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
    pParent->SendMessageA(WM_SEND_PACKET, 8 << 1 | 1);
}
