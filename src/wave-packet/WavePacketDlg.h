// WavePacketDlg.h : header file
//

#pragma once

#include <util/common/gui/SimulationDialog.h>
#include <util/common/plot/PlotStatic.h>

const size_t max_wavefuncs = 6;

// CWavePacketDlg dialog
class CWavePacketDlg : public CSimulationDialog
{
// Construction
public:
	CWavePacketDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_WAVEPACKET_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
    virtual void OnSimulation();
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();
    double m_lfBarrierWidth;
    double m_lfBarrierHeight;
    double m_lfModelingInterval;
    double m_lfTimeDelta;
    double m_lfDistanceDelta;
    PlotStatic m_cWavePacketPlot;
    PlotStatic m_cSpectrumPlot;
    PlotStatic m_cWaveFunctionPlot;
    int m_nOrbitalMomentum;
};
