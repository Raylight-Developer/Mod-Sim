#include "Viewport.hpp"


Graphics_View::Graphics_View(QWidget* parent) :
	QGraphicsView(parent)
{
	setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::FullViewportUpdate);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setAttribute(Qt::WidgetAttribute::WA_NativeWindow);

	setRenderHint(QPainter::RenderHint::TextAntialiasing);
	setRenderHint(QPainter::RenderHint::Antialiasing);

	setContentsMargins(0, 0, 0, 0);

	QSurfaceFormat format;
	format.setSwapInterval(1);
	QSurfaceFormat::setDefaultFormat(format);

	setMaxSize();
	centerOn(0, 0);

	zoom_sensitivity = 0.2;
	is_panning = false;
	is_zooming = false;
	view_scale = 1.0;
}

void Graphics_View::keyPressEvent(QKeyEvent * event) {
	if (event->key() == Qt::Key_Z){
		is_zooming = true;
	}

	if (is_zooming){
		if (event->key() == Qt::Key_Up)
			zoomIn();

		else if (event->key() == Qt::Key_Down)
			zoomOut();
	}

	else {
		QGraphicsView::keyPressEvent(event);
	}
}

void Graphics_View::keyReleaseEvent(QKeyEvent * event) {
	if (event->key() == Qt::MiddleButton){
		is_zooming = false;
	}

	QGraphicsView::keyReleaseEvent(event);
}

void Graphics_View::mouseMoveEvent(QMouseEvent * event) {
	if (is_panning) {
		pan(mapToScene(event->pos()) - mapToScene(last_mouse));
	}

	QGraphicsView::mouseMoveEvent(event);
	last_mouse = event->pos();
}

void Graphics_View::mousePressEvent(QMouseEvent * event) {
	if (event->button() == Qt::MiddleButton){
		last_mouse = event->pos();
		is_panning = true;
		setCursor(Qt::ClosedHandCursor);
	}

	QGraphicsView::mousePressEvent(event);
}

void Graphics_View::mouseReleaseEvent(QMouseEvent * event) {
	if (event->button() == Qt::MiddleButton){
		is_panning = false;
		setCursor(Qt::ArrowCursor);
	}

	QGraphicsView::mouseReleaseEvent(event);
}

void Graphics_View::wheelEvent(QWheelEvent *event) {
	const QPoint scrollAmount = event->angleDelta();
	scrollAmount.y() > 0 ? zoomIn() : zoomOut();
}

void Graphics_View::setMaxSize() {
	setSceneRect(INT_MIN/2, INT_MIN/2, INT_MAX, INT_MAX);
}

void Graphics_View::zoom(float scaleFactor) {
	scale(scaleFactor, scaleFactor);
	view_scale *= scaleFactor;
}

void Graphics_View::zoomIn() {
	zoom(1 + zoom_sensitivity);
}

void Graphics_View::zoomOut() {
	zoom (1 - zoom_sensitivity);
}

void Graphics_View::pan(QPointF delta) {
	delta *= view_scale;

	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	QPoint newCenter(viewport()->rect().width() / 2 - delta.x(),  viewport()->rect().height() / 2 + delta.y());
	centerOn(mapToScene(newCenter));

	setTransformationAnchor(QGraphicsView::AnchorViewCenter);
}