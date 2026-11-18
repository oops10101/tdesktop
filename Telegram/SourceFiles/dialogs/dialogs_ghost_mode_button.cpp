/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "dialogs/dialogs_ghost_mode_button.h"

#include "ui/effects/ripple_animation.h"
#include "ui/painter.h"
#include "styles/style_dialogs.h"

#include <QtGui/QPainterPath>

#include <cmath>

namespace Dialogs {
namespace {

constexpr auto kActiveDuration = crl::time(280);
constexpr auto kOverDuration = crl::time(150);
constexpr auto kPi = 3.14159265358979323846;

// Round head, straight sides and a skirt of three rounded tails.
[[nodiscard]] QPainterPath GhostBodyPath(const QRectF &r) {
	const auto w = r.width();
	const auto h = r.height();
	const auto tails = 3;
	const auto tail = w / tails;
	const auto depth = h * 0.14;
	const auto base = r.y() + h - depth;

	auto result = QPainterPath();
	result.moveTo(r.x(), r.y() + w / 2.);
	result.arcTo(QRectF(r.x(), r.y(), w, w), 180., -180.);
	result.lineTo(r.x() + w, base);
	for (auto i = 0; i != tails; ++i) {
		const auto from = r.x() + w - i * tail;
		const auto to = from - tail;
		result.cubicTo(
			QPointF(from - tail * 0.1, base + depth * 1.33),
			QPointF(to + tail * 0.1, base + depth * 1.33),
			QPointF(to, base));
	}
	result.closeSubpath();
	return result;
}

[[nodiscard]] QPainterPath GhostEyesPath(const QRectF &r) {
	const auto w = r.width();
	const auto eye = QSizeF(w * 0.17, w * 0.26);
	const auto center = QPointF(r.x() + w / 2., r.y() + w * 0.5);
	const auto shift = w * 0.2;

	auto result = QPainterPath();
	for (const auto side : { -1., 1. }) {
		result.addEllipse(QRectF(
			center.x() + side * shift - eye.width() / 2.,
			center.y() - eye.height() / 2.,
			eye.width(),
			eye.height()));
	}
	return result;
}

[[nodiscard]] float64 EaseOutCubic(float64 t) {
	const auto inverted = 1. - t;
	return 1. - inverted * inverted * inverted;
}

} // namespace

GhostModeButton::GhostModeButton(QWidget *parent)
: RippleButton(parent, st::dialogsMenuToggle.ripple) {
	resize(st::dialogsMenuToggle.width, st::dialogsMenuToggle.height);
}

void GhostModeButton::setActive(bool active, anim::type animated) {
	if (_active == active) {
		return;
	}
	_active = active;
	if (animated == anim::type::normal) {
		_activeAnimation.start(
			[=] { update(); },
			active ? 0. : 1.,
			active ? 1. : 0.,
			kActiveDuration);
	} else {
		_activeAnimation.stop();
		update();
	}
}

void GhostModeButton::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);

	const auto active = _activeAnimation.value(_active ? 1. : 0.);
	const auto over = _overAnimation.value(isOver() ? 1. : 0.);
	const auto area = rippleRect();
	const auto center = QRectF(area).center();

	paintRipple(p, area.topLeft());

	PainterHighQualityEnabler hq(p);
	if (active > 0.) {
		// Accent badge for the white ghost, with a small pop when it lands.
		const auto full = area.width() / 2. - style::ConvertScaleExact(4.);
		const auto pop = 1. + 0.1 * std::sin(kPi * active);
		const auto radius = full * EaseOutCubic(active) * pop;
		p.setPen(Qt::NoPen);
		p.setBrush(anim::color(
			st::activeButtonBg,
			st::activeButtonBgOver,
			over));
		p.drawEllipse(center, radius, radius);

		// Soft halo around the badge.
		auto halo = st::activeButtonBg->c;
		halo.setAlphaF(0.28 * active);
		const auto ring = radius + style::ConvertScaleExact(2.5);
		p.setPen(QPen(halo, style::ConvertScaleExact(1.5)));
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(center, ring, ring);
	}

	const auto size = QSizeF(
		style::ConvertScaleExact(16.),
		style::ConvertScaleExact(18.));
	const auto hop = style::ConvertScaleExact(2.) * std::sin(kPi * active);
	const auto rect = QRectF(
		center.x() - size.width() / 2.,
		(center.y()
			- size.height() / 2.
			- hop
			+ style::ConvertScaleExact(0.5)),
		size.width(),
		size.height());
	const auto body = GhostBodyPath(rect);
	const auto eyes = GhostEyesPath(rect);

	if (active < 1.) {
		// Off: hollow ghost with a faint tint, in the regular icon color.
		const auto color = anim::color(
			st::dialogsMenuIconFg,
			st::dialogsMenuIconFgOver,
			over);
		auto tint = color;
		tint.setAlphaF(color.alphaF() * 0.14);
		auto pen = QPen(color, style::ConvertScaleExact(1.6));
		pen.setJoinStyle(Qt::RoundJoin);
		p.setOpacity(1. - active);
		p.setPen(pen);
		p.setBrush(tint);
		p.drawPath(body);
		p.setPen(Qt::NoPen);
		p.setBrush(color);
		p.drawPath(eyes);
	}
	if (active > 0.) {
		// On: solid white ghost with the eyes cut out.
		p.setOpacity(active);
		p.setPen(Qt::NoPen);
		p.setBrush(st::activeButtonFg);
		p.drawPath(body.subtracted(eyes));
	}
}

void GhostModeButton::onStateChanged(State was, StateChangeSource source) {
	RippleButton::onStateChanged(was, source);

	const auto over = isOver();
	const auto wasOver = static_cast<bool>(was & StateFlag::Over);
	if (over != wasOver) {
		_overAnimation.start(
			[=] { update(); },
			over ? 0. : 1.,
			over ? 1. : 0.,
			kOverDuration);
	}
}

QRect GhostModeButton::rippleRect() const {
	const auto size = st::dialogsMenuToggle.rippleAreaSize;
	return QRect(
		(width() - size) / 2,
		(height() - size) / 2,
		size,
		size);
}

QImage GhostModeButton::prepareRippleMask() const {
	return ::Ui::RippleAnimation::EllipseMask(rippleRect().size());
}

QPoint GhostModeButton::prepareRippleStartPosition() const {
	return mapFromGlobal(QCursor::pos()) - rippleRect().topLeft();
}

} // namespace Dialogs
