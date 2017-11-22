
// WavePacketDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wave-packet.h"
#include "WavePacketDlg.h"
#include "afxdialogex.h"

#include <util/common/iterable.h>

#include "model.h"

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

std::vector < double > energy_levels;

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
                return point < double >
                {
                    j * m_lfDistanceDelta,
                    (j == 0) ? 0 : norm(*it) / m_lfDistanceDelta / j
                };
            }
        ))
        .with_auto_viewport(wave_packet_avp);

    spectrum_plot
        .with_view()
        .with_view_line_pen(plot::palette::pen(RGB(255, 255, 255), 2))
        .with_data(make_container_mapping_iterable < point < double > > (
            create < points_t > (),
            [&] (const points_t & c, const points_t::const_iterator & it, size_t j)
            {
                return point < double >
                {
                    (double) j / c.size() / m_lfTimeDelta / 2,
                    norm(*it)
                };
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
                    return point < double > { j * m_lfDistanceDelta, it->re };
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

    custom_drawable::ptr_t barrier_strobes = custom_drawable::create
    (
        [&] (CDC & dc, const viewport & vp)
        {
            auto pen = palette::pen(0xffffff, 3);
            dc.SelectObject(pen.get());
            dc.MoveTo(vp.world_to_screen().x(m_lfBarrierWidth), vp.screen.ymax);
            dc.LineTo(vp.world_to_screen().x(m_lfBarrierWidth), vp.screen.ymax - 15);
            dc.MoveTo(vp.world_to_screen().x((m_lfBarrierWidth + m_lfModelingInterval) / 2), vp.screen.ymax);
            dc.LineTo(vp.world_to_screen().x((m_lfBarrierWidth + m_lfModelingInterval) / 2), vp.screen.ymax - 15);
        }
    );

    m_cWavePacketPlot.plot_layer.with(
        viewporter::create(
            tick_drawable::create(
                layer_drawable::create(std::vector < drawable::ptr_t > ({
                    wave_packet_plot.view, barrier_strobes
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

    custom_drawable::ptr_t energy_level_strobes = custom_drawable::create
    (
        [&] (CDC & dc, const viewport & vp)
        {
            auto pen = palette::pen(0xffffff, 3);
            dc.SelectObject(pen.get());
            for (size_t i = 0; i < energy_levels.size(); ++i)
            {
                dc.MoveTo(vp.world_to_screen().x(energy_levels[i]), vp.screen.ymin);
                dc.LineTo(vp.world_to_screen().x(energy_levels[i]), vp.screen.ymin + 15);
            }
            dc.MoveTo(vp.world_to_screen().x(m_lfBarrierHeight), vp.screen.ymax);
            dc.LineTo(vp.world_to_screen().x(m_lfBarrierHeight), vp.screen.ymax - 15);
        }
    );

    m_cSpectrumPlot.plot_layer.with(
        viewporter::create(
            tick_drawable::create(
                layer_drawable::create(std::vector < drawable::ptr_t > ({
                    spectrum_plot.view, energy_level_strobes
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

    wavefunc_layers.push_back(barrier_strobes);
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
    auto & wave_packet = * wave_packet_plot.data->container_ptr;
    auto & spectrum    = * spectrum_plot.data->container_ptr;

    const size_t max_energy_levels = max_wavefuncs;

    std::vector < std::vector < complex < double > > > wave_packets;

    // fill wave packet with default data

    size_t n = std::ceil(m_lfModelingInterval / m_lfDistanceDelta);

    wavefunc_fixed_bound->xmin = 0;
    wavefunc_fixed_bound->xmax = m_lfModelingInterval;

    wave_packet.resize(n);
    for (size_t i = 0; i < wave_packet.size(); ++i)
    {
        double x = i * m_lfDistanceDelta;
        wave_packet[i] = m_lfPacketMagnitude * exp(- (x - m_lfPacketPosition) * (x - m_lfPacketPosition) / m_lfPacketDispersion / m_lfPacketDispersion);
    }

    // make potential and boundary condition functions

    auto p = model::make_potential_fn(m_lfBarrierWidth, m_lfBarrierHeight, m_nOrbitalMomentum);
    auto s = model::make_free_space_condition_fn((m_lfBarrierWidth + m_lfModelingInterval) / 2, m_lfGamma);

    // init common data

    tmm_data data;
    tmm_make_data(data, m_lfTimeDelta, 0, m_lfModelingInterval, m_lfDistanceDelta, p, s);

    size_t fixed_point = rand() % n;

    // do while not interrupted

    wave_packet_plot.refresh();
    m_cWavePacketPlot.RedrawBuffer();
    m_cWavePacketPlot.SwapBuffers();
    Invoke([&] () {
        m_cWavePacketPlot.RedrawWindow();
    });

    energy_levels.clear();
    std::vector < size_t > energy_level_indices;

    size_t t = 0;

    while (m_bWorking)
    {
        // calculate next wave packet state

        tmm_solve(data, wave_packet, wave_packet, 0, 0, 0, 0);

        // if enough time passed, start recording wave packet
        // state at the fixed point

        if (t >= m_nSpectrumPoints)
        {
            wave_packets.push_back(std::vector < complex < double > > (wave_packet));
        }

        // if enough data collected at the fixed point,
        // calculate fourier transform and get a list
        // of frequencies to watch for later to reconstruct
        // spatial wave functions

        if (wave_packets.size() >= m_nSpectrumPoints)
        {
            spectrum.resize(wave_packets.size());
            for (size_t i = 0; (i < wave_packets.size()) && m_bWorking; ++i)
            {
                spectrum[i] = 0;
                for (size_t j = 0; (j < wave_packets.size()) && m_bWorking; ++j)
                {
                    spectrum[i] = spectrum[i] + wave_packets[j][fixed_point]
                        * exp(- _i * (double)i / wave_packets.size() * (1. / m_lfTimeDelta / 2)
                              * ((int)t - (int)wave_packets.size() + j) * m_lfTimeDelta);
                }
                spectrum[i] = spectrum[i] / fixed_point / m_lfDistanceDelta;
            }

            double mean_spectrum = 0;
            std::vector < double > spectrum_norm(spectrum.size());
            for (size_t i = 0; i < spectrum.size(); ++i)
            {
                spectrum_norm[i] = sqnorm(spectrum[i]);
                mean_spectrum += spectrum_norm[i];
            }

            mean_spectrum /= spectrum.size();

            energy_levels.clear();

            bool has_any;
            do
            {
                double max_val = 0;
                size_t max_idx = 0;

                // find a neighborhood of the maximum

                for (size_t i = 0; i < spectrum.size(); ++i)
                {
                    if (spectrum_norm[i] > max_val)
                    {
                        max_val = spectrum_norm[i];
                        max_idx = i;
                        has_any = true;
                    }
                }

                if (max_val < mean_spectrum * 2) has_any = false;

                if (!has_any) break;

                energy_levels.push_back((double) max_idx / spectrum.size()
                                        * (1 / m_lfTimeDelta / 2));
                energy_level_indices.push_back(max_idx);

                // cut a neighborhood of the maximum

                double thr_r = max_val, thr_l = max_val;
                size_t peak_width_r = 0, peak_width_l = 0;

                for (size_t i = max_idx + 1; i < spectrum.size(); ++i)
                {
                    if (spectrum_norm[i] <= thr_r)
                    {
                        thr_r = spectrum_norm[i];
                        ++peak_width_r;
                        continue;
                    }
                    break;
                }

                for (size_t i = max_idx; i-- > 0;)
                {
                    if (spectrum_norm[i] <= thr_l)
                    {
                        thr_l = spectrum_norm[i];
                        ++peak_width_l;
                        continue;
                    }
                    break;
                }

                for (size_t i = max_idx; i < min(max_idx + peak_width_r * 3, spectrum.size()); ++i)
                {
                    spectrum_norm[i] = 0;
                }

                for (size_t i = max(0, (int)max_idx - (int)peak_width_l * 3); i < max_idx; ++i)
                {
                    spectrum_norm[i] = 0;
                }
            } while (has_any && (energy_levels.size() < max_energy_levels));

            // refine energy levels found

            auto spectrum_fn = [&] (double e) -> double
            {
                complex < double > val = 0;
                for (size_t j = 0; (j < wave_packets.size()) && m_bWorking; ++j)
                {
                    val = val + wave_packets[j][fixed_point]
                        * exp(- _i * e * ((int)t - (int)wave_packets.size() + j) * m_lfTimeDelta);
                }
                return norm(val);
            };

            // apply gold section method

            const double gs = 0.618;
            for (size_t i = 0; i < energy_levels.size(); ++i)
            {
                double e = energy_levels[i];
                double left_e = e - (1 / m_lfTimeDelta / wave_packets.size() / 2);
                double right_e = e + (1 / m_lfTimeDelta / wave_packets.size() / 2);
                double e1 = right_e - (right_e - left_e) * gs;
                double e2 = left_e + (right_e - left_e) * gs;
                double val1 = spectrum_fn(e1), val2 = spectrum_fn(e2);
                do
                {
                    if (val1 <= val2)
                    {
                        left_e = e1;
                        e1 = e2;
                        e2 = left_e + (right_e - left_e) * gs;
                        val2 = spectrum_fn(e2);
                    }
                    else
                    {
                        right_e = e2;
                        e2 = e1;
                        e1 = right_e - (right_e - left_e) * gs;
                        val1 = spectrum_fn(e1);
                    }
                } while ((right_e - left_e) > 1e-8);
                energy_levels[i] = (right_e + left_e) / 2;
            }

            spectrum_plot.refresh();
            m_cSpectrumPlot.RedrawBuffer();
            m_cSpectrumPlot.SwapBuffers();
            Invoke([&] () {
                m_cSpectrumPlot.RedrawWindow();
            });

            if (!energy_levels.empty())
            {
                for (size_t i = 0; i < max_wavefuncs; ++i)
                {
                    wavefunc_plots[i].data->container_ptr->clear();
                }
                for (size_t i = 0; i < energy_levels.size(); ++i)
                {
                    wavefunc_plots[i].data->container_ptr->resize(n);
                }

                for (size_t k = 0; k < n; ++k)
                {
                    for (size_t i = 0; i < energy_levels.size(); ++i)
                    {
                        complex < double > val = 0;
                        if (k != 0)
                        {
                            for (size_t j = 0; (j < wave_packets.size()) && m_bWorking; ++j)
                            {
                                val = val + wave_packets[j][k]
                                    * exp(- _i * energy_levels[i]
                                          * ((int)t - (int)wave_packets.size() + j) * m_lfTimeDelta);
                            }
                            val = val / k;
                        }
                        wavefunc_plots[i].data->container_ptr->at(k) = val;
                    }
                }

                wavefunc_plots[0].auto_world->clear();
                if (m_nWaveFunctionToDisplay > energy_levels.size())
                {
                    m_nWaveFunctionToDisplay = energy_levels.size();
                }
                if (m_nWaveFunctionToDisplay <= 0)
                {
                    for (size_t i = 0; i < energy_levels.size(); ++i)
                    {
                        wavefunc_plots[i].view->visible = true;
                        wavefunc_plots[i].auto_world->adjust(*wavefunc_plots[i].data);
                    }
                }
                else
                {
                    for (size_t i = 0; i < energy_levels.size(); ++i)
                    {
                        wavefunc_plots[i].view->visible = ((i + 1) == m_nWaveFunctionToDisplay);
                        if ((i + 1) == m_nWaveFunctionToDisplay)
                        {
                            wavefunc_plots[i].auto_world->adjust(*wavefunc_plots[i].data);
                        }
                    }
                }
                wavefunc_plots[0].auto_world->flush();
                m_cWaveFunctionPlot.RedrawBuffer();
                m_cWaveFunctionPlot.SwapBuffers();
                Invoke([&] () {
                    m_cWaveFunctionPlot.RedrawWindow();
                });
            }

            wave_packets.clear();
        }

        ++t;

        wave_packet_plot.refresh();
        m_cWavePacketPlot.RedrawBuffer();
        m_cWavePacketPlot.SwapBuffers();
        Invoke([&] () {
            m_cWavePacketPlot.RedrawWindow();
        });
    }

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

    if (m_nWaveFunctionToDisplay >= energy_levels.size())
    {
        m_nWaveFunctionToDisplay = energy_levels.size();
    }
    wavefunc_plots[0].auto_world->clear();
    if (m_nWaveFunctionToDisplay <= 0)
    {
        for (size_t i = 0; i < energy_levels.size(); ++i)
        {
            wavefunc_plots[i].view->visible = true;
            wavefunc_plots[i].auto_world->adjust(*wavefunc_plots[i].data);
        }
    }
    else
    {
        for (size_t i = 0; i < energy_levels.size(); ++i)
        {
            wavefunc_plots[i].view->visible = ((i + 1) == m_nWaveFunctionToDisplay);
            if ((i + 1) == m_nWaveFunctionToDisplay)
            {
                wavefunc_plots[i].auto_world->adjust(*wavefunc_plots[i].data);
            }
        }
    }
    wavefunc_plots[0].auto_world->flush();
    m_cWaveFunctionPlot.RedrawBuffer();
    m_cWaveFunctionPlot.SwapBuffers();
    m_cWaveFunctionPlot.RedrawWindow();
}
