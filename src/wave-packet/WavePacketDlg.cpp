
// WavePacketDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wave-packet.h"
#include "WavePacketDlg.h"
#include "afxdialogex.h"

#include <util/common/iterable.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWavePacketDlg dialog

using namespace plot;
using namespace util;
using namespace math;
using namespace model;

using points_t = std::vector < complex < double > > ;
using data_t   = container_mapping_iterable_t < points_t, point < double > > ;
using plot_t   = simple_list_plot < data_t > ;

plot_t wave_packet_plot
     , spectrum_plot
     , wavefunc_plots[max_wavefuncs]
     ;


world_t::ptr_t wavefunc_fixed_bound = world_t::create();

CWavePacketDlg::CWavePacketDlg(CWnd* pParent /*=NULL*/)
	: CSimulationDialog(CWavePacketDlg::IDD, pParent)
    , m_lfBarrierWidth(1)
    , m_lfBarrierHeight(1)
    , m_lfModelingInterval(2)
    , m_lfTimeDelta(0.001)
    , m_lfDistanceDelta(0.001)
    , m_nOrbitalMomentum(0)
    , m_lfGamma(1)
    , m_lfPacketMagnitude(1)
    , m_lfPacketDispersion(0.1)
    , m_lfPacketPosition(0.5)
    , m_nSpectrumPoints(1000)
    , m_nWaveFunctionToDisplay(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWavePacketDlg::DoDataExchange(CDataExchange* pDX)
{
	CSimulationDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT1, m_lfBarrierWidth);
    DDX_Text(pDX, IDC_EDIT2, m_lfBarrierHeight);
    DDX_Text(pDX, IDC_EDIT3, m_lfModelingInterval);
    DDX_Text(pDX, IDC_EDIT4, m_lfTimeDelta);
    DDX_Text(pDX, IDC_EDIT5, m_lfDistanceDelta);
    DDX_Control(pDX, IDC_PLOT1, m_cWavePacketPlot);
    DDX_Control(pDX, IDC_PLOT2, m_cSpectrumPlot);
    DDX_Control(pDX, IDC_PLOT3, m_cWaveFunctionPlot);
    DDX_Text(pDX, IDC_EDIT6, m_nOrbitalMomentum);
    DDX_Text(pDX, IDC_EDIT7, m_lfGamma);
    DDX_Text(pDX, IDC_EDIT8, m_lfPacketMagnitude);
    DDX_Text(pDX, IDC_EDIT9, m_lfPacketDispersion);
    DDX_Text(pDX, IDC_EDIT10, m_lfPacketPosition);
    DDX_Text(pDX, IDC_EDIT11, m_nSpectrumPoints);
    DDX_Text(pDX, IDC_EDIT12, m_nWaveFunctionToDisplay);
}

BEGIN_MESSAGE_MAP(CWavePacketDlg, CSimulationDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CWavePacketDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CWavePacketDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON3, &CWavePacketDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CWavePacketDlg message handlers

BOOL CWavePacketDlg::OnInitDialog()
{
	CSimulationDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

    auto_viewport_params params;
    params.factors = { 0, 0, 0.1, 0.1 };
    auto wave_packet_avp = min_max_auto_viewport < data_t > :: create();
    auto spectrum_avp    = min_max_auto_viewport < data_t > :: create();
    auto wavefunc_avp    = min_max_auto_viewport < data_t > :: create();
    wave_packet_avp->set_params(params);
    spectrum_avp->set_params(params);
    params.fixed = { true, true, false, false };
    params.fixed_bound = wavefunc_fixed_bound;
    wavefunc_avp->set_params(params);

    wave_packet_plot
        .with_view()
        .with_view_line_pen(plot::palette::pen(RGB(255, 255, 255), 2))
        .with_data(make_container_mapping_iterable < point < double > > (
            create < points_t > (),
            [&] (const points_t &, const points_t::const_iterator & it, size_t j)
            {
                return point < double > { j * m_lfDistanceDelta, norm(*it) };
            }
        ))
        .with_auto_viewport(wave_packet_avp);

    spectrum_plot
        .with_view()
        .with_view_line_pen(plot::palette::pen(RGB(255, 255, 255), 2))
        .with_data(make_container_mapping_iterable < point < double > > (
            create < points_t > (),
            [&] (const points_t &, const points_t::const_iterator & it, size_t j)
            {
                return point < double > { (double) j / m_lfTimeDelta, norm(*it) };
            }
        ))
        .with_auto_viewport(wave_packet_avp);

    std::vector < drawable::ptr_t > wavefunc_layers;

    for (size_t i = 0; i < max_wavefuncs; ++i)
    {
        wavefunc_plots[i]
            .with_view()
            .with_view_line_pen(plot::palette::pen(RGB(127 + rand() % 128, 127 + rand() % 128, 255), 2))
            .with_data(make_container_mapping_iterable < point < double > > (
                create < points_t > (),
                [&] (const points_t &, const points_t::const_iterator & it, size_t j)
                {
                    return point < double > { j * m_lfDistanceDelta, norm(*it) };
                }
            ))
            .with_auto_viewport(wavefunc_avp);
        wavefunc_layers.push_back(wavefunc_plots[i].view);
    }

    m_cWavePacketPlot.background   = palette::brush();
    m_cSpectrumPlot.background     = palette::brush();
    m_cWaveFunctionPlot.background = palette::brush();

    m_cWavePacketPlot.triple_buffered   = true;
    m_cSpectrumPlot.triple_buffered     = true;
    m_cWaveFunctionPlot.triple_buffered = true;

    m_cWavePacketPlot.plot_layer.with(
        viewporter::create(
            tick_drawable::create(
                layer_drawable::create(std::vector < drawable::ptr_t > ({
                    wave_packet_plot.view
                })),
                const_n_tick_factory<axe::x>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                const_n_tick_factory<axe::y>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                palette::pen(RGB(80, 80, 80)),
                RGB(200, 200, 200)
            ),
            make_viewport_mapper(wave_packet_plot.viewport_mapper)
        )
    );

    m_cSpectrumPlot.plot_layer.with(
        viewporter::create(
            tick_drawable::create(
                layer_drawable::create(std::vector < drawable::ptr_t > ({
                    spectrum_plot.view
                })),
                const_n_tick_factory<axe::x>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                const_n_tick_factory<axe::y>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                palette::pen(RGB(80, 80, 80)),
                RGB(200, 200, 200)
            ),
            make_viewport_mapper(spectrum_plot.viewport_mapper)
        )
    );

    m_cWaveFunctionPlot.plot_layer.with(
        viewporter::create(
            tick_drawable::create(
                layer_drawable::create(wavefunc_layers),
                const_n_tick_factory<axe::x>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                const_n_tick_factory<axe::y>::create(
                    make_simple_tick_formatter(2, 5),
                    0,
                    5
                ),
                palette::pen(RGB(80, 80, 80)),
                RGB(200, 200, 200)
            ),
            make_viewport_mapper(wavefunc_plots[0].viewport_mapper)
        )
    );

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWavePacketDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CSimulationDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWavePacketDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CWavePacketDlg::OnSimulation()
{
    CSimulationDialog::OnSimulation();
}


void CWavePacketDlg::OnBnClickedButton1()
{
    UpdateData(TRUE);
    StartSimulationThread();
}


void CWavePacketDlg::OnBnClickedButton2()
{
    StopSimulationThread();
}


void CWavePacketDlg::OnBnClickedButton3()
{
    UpdateData(TRUE);
}
