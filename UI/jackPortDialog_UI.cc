#include <gtk/gtk.h>
#include <iostream>
#include "jackPortDialog.h"



bool JackPortDialog::updateGraph(const Cairo::RefPtr<Cairo::Context> &context)
{
    if (jackPortDialogDrawingArea == nullptr) {
        return true;
    }//if

    if (false == jackPortDialogDrawingArea->get_realized()) {
    	return false;
    }//if

    Glib::RefPtr<Gdk::Window> drawingAreaWindow = jackPortDialogDrawingArea->get_window();

    int width = jackPortDialogDrawingArea->get_width();
    int height = jackPortDialogDrawingArea->get_height();

    context->save();

    context->reset_clip();
    context->rectangle(0, 0, width, height);
    context->clip();

    context->set_source_rgba(0.0, 1.0, 0.0, 1.0);
    context->paint();

    context->reset_clip();
    context->set_source_rgba(0.9, 0.0, 0.0, 1.0);
    context->set_line_width(1.0);

    for (int x = 0; x < width; x += 50) {
        context->move_to(x, 0);
        context->line_to(x, height);
        context->stroke();
    }//for

    context->set_source_rgba(0.2, 0.0, 0.7, 1.0);
    for (int y = 0; y < height; y += 50) {
        context->move_to(0, y);
        context->line_to(width, y);
        context->stroke();
    }//for

    context->restore();

    return true;
}//updateGraph

bool JackPortDialog::mouseButtonPressed(GdkEventButton *event)
{
    return true;
}//mouseButtonPressed

bool JackPortDialog::mouseButtonReleased(GdkEventButton *event)
{
    return true;
}//mouseButtonReleased

bool JackPortDialog::mouseMoved(GdkEventMotion *event)
{
    return true;
}//mouseMoved



