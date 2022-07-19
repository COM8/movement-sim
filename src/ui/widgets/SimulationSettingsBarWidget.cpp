#include "SimulationSettingsBarWidget.hpp"
#include "sim/Simulator.hpp"
#include "ui/widgets/SimulationWidget.hpp"
#include <cassert>
#include <gdkmm/display.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/box.h>
#include <gtkmm/enums.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/image.h>

namespace ui::widgets {
SimulationSettingsBarWidget::SimulationSettingsBarWidget(SimulationWidget* simWidget, SimulationOverlayWidget* simOverlayWidget) : Gtk::Box(Gtk::Orientation::HORIZONTAL),
                                                                                                                                   simWidget(simWidget),
                                                                                                                                   simOverlayWidget(simOverlayWidget),
                                                                                                                                   simulator(sim::Simulator::get_instance()) {
    prep_widget();
}

void SimulationSettingsBarWidget::prep_widget() {
    mainBox.add_css_class("linked");
    mainBox.set_margin_start(10);
    append(mainBox);

    zoomBox.add_css_class("linked");
    zoomBox.set_margin_start(10);
    append(zoomBox);

    miscBox.add_css_class("linked");
    miscBox.set_margin_start(10);
    append(miscBox);

    simulateTBtn.property_active().signal_changed().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_simulate_toggled));
    simulateTBtn.set_icon_name("play-large-symbolic");
    simulateTBtn.set_tooltip_text("Enable simulation");
    simulateTBtn.add_css_class("suggested-action");
    mainBox.append(simulateTBtn);

    renderTBtn.property_active().signal_changed().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_render_toggled));
    renderTBtn.set_active();
    renderTBtn.set_icon_name("display-with-window-symbolic");
    renderTBtn.set_tooltip_text("Enable renderer");
    mainBox.append(renderTBtn);

    debugOverlayTBtn.property_active().signal_changed().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_debug_overlay_toggled));
    debugOverlayTBtn.set_icon_name("closed-captioning-symbolic");

    // Glib::RefPtr<Gtk::IconPaintable> icon = Gtk::IconTheme::get_for_display(Gdk::Display::get_default())->lookup_icon("list-symbolic", 64);
    // Gtk::Image* iconImage = Gtk::make_managed<Gtk::Image>();
    // iconImage->set_from_icon_name(icon->get_icon_name());
    // debugOverlayTBtn.set_child(*iconImage);
    // debugOverlayTBtn.set_icon_name(icon->get_icon_name());

    debugOverlayTBtn.set_active();
    debugOverlayTBtn.set_tooltip_text("Toggle debug overlay");
    mainBox.append(debugOverlayTBtn);

    zoomInBtn.signal_clicked().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_zoom_in_clicked));
    zoomInBtn.set_tooltip_text("Zoom in");
    zoomInBtn.set_icon_name("zoom-in");
    zoomInBtn.set_sensitive(false);
    zoomBox.append(zoomInBtn);

    zoomOutBtn.signal_clicked().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_zoom_out_clicked));
    zoomOutBtn.set_tooltip_text("Zoom out");
    zoomOutBtn.set_icon_name("zoom-out");
    zoomBox.append(zoomOutBtn);

    zoomFitBtn.signal_clicked().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_zoom_fit_clicked));
    zoomFitBtn.set_tooltip_text("Zoom fit");
    zoomFitBtn.set_icon_name("zoom-fit-best");
    zoomBox.append(zoomFitBtn);

    zoomResetBtn.signal_clicked().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_zoom_reset_clicked));
    zoomResetBtn.set_tooltip_text("Zoom reset");
    zoomResetBtn.set_icon_name("zoom-original");
    zoomBox.append(zoomResetBtn);

    blurTBtn.property_active().signal_changed().connect(sigc::mem_fun(*this, &SimulationSettingsBarWidget::on_blur_toggled));
    blurTBtn.set_icon_name("blur-symbolic");
    blurTBtn.set_tooltip_text("Blur entities");
    miscBox.append(blurTBtn);
}

//-----------------------------Events:-----------------------------
void SimulationSettingsBarWidget::on_simulate_toggled() {
    assert(simulator);
    if (simulateTBtn.get_active()) {
        simulator->continue_simulation();
        simulateTBtn.set_icon_name("pause-large-symbolic");
    } else {
        simulator->pause_simulation();
        simulateTBtn.set_icon_name("play-large-symbolic");
    }
}

void SimulationSettingsBarWidget::on_render_toggled() {
    assert(simWidget);
    simWidget->enableUiUpdates = renderTBtn.get_active();
}

void SimulationSettingsBarWidget::on_debug_overlay_toggled() {
    assert(simOverlayWidget);
    simOverlayWidget->set_debug_overlay_enabled(debugOverlayTBtn.get_active());
}

void SimulationSettingsBarWidget::on_zoom_in_clicked() {
    assert(simWidget);
    float zoomFactor = simWidget->get_zoom_factor();
    zoomFactor *= 1.25;
    if (zoomFactor >= 1) {
        zoomFactor = 1;
        zoomInBtn.set_sensitive(false);
    }
    simWidget->set_zoom_factor(zoomFactor);
}

void SimulationSettingsBarWidget::on_zoom_out_clicked() {
    assert(simWidget);
    float zoomFactor = simWidget->get_zoom_factor();
    zoomFactor *= 0.75;
    zoomInBtn.set_sensitive(true);
    simWidget->set_zoom_factor(zoomFactor);
}

void SimulationSettingsBarWidget::on_zoom_reset_clicked() {
    assert(simWidget);
    simWidget->set_zoom_factor(1.0);
    zoomInBtn.set_sensitive(false);
}

void SimulationSettingsBarWidget::on_zoom_fit_clicked() {
    assert(simWidget);
    float zoomFactor = simWidget->get_zoom_factor();
    float zoomX = static_cast<float>(simWidget->get_width()) / sim::MAX_RENDER_RESOLUTION_X;
    float zoomY = static_cast<float>(simWidget->get_height()) / sim::MAX_RENDER_RESOLUTION_Y;
    zoomFactor = zoomX > zoomY ? zoomY : zoomX;
    simWidget->set_zoom_factor(zoomFactor);
    zoomInBtn.set_sensitive(true);
}

void SimulationSettingsBarWidget::on_blur_toggled() {
    assert(simWidget);
    simWidget->set_blur(blurTBtn.get_active());
}
}  // namespace ui::widgets
