#pragma once

#include "Include.hpp"

struct Graphics_View : QGraphicsView {
	qreal zoom_sensitivity;
	qreal view_scale;

	void pan(QPointF delta);
	void zoom(float scaleFactor);
	void zoomIn();
	void zoomOut();

	Graphics_View(QWidget* parent = nullptr);

	void keyPressEvent(QKeyEvent*) override;
	void keyReleaseEvent(QKeyEvent*) override;

	void mouseMoveEvent(QMouseEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void wheelEvent(QWheelEvent*) override;

private:
	bool is_panning;
	bool is_zooming;

	QPoint last_mouse;
	void setMaxSize();
};