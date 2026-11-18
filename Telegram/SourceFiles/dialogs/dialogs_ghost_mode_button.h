/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/widgets/buttons.h"
#include "ui/effects/animations.h"

namespace Dialogs {

// Ghost mode switch next to the chats search field. Off: a hollow ghost
// in the regular icon color. On: a white ghost on an accent badge.
class GhostModeButton final : public ::Ui::RippleButton {
public:
	explicit GhostModeButton(QWidget *parent);

	void setActive(bool active, anim::type animated = anim::type::normal);
	[[nodiscard]] bool active() const {
		return _active;
	}

protected:
	void paintEvent(QPaintEvent *e) override;
	void onStateChanged(State was, StateChangeSource source) override;

	QImage prepareRippleMask() const override;
	QPoint prepareRippleStartPosition() const override;

private:
	[[nodiscard]] QRect rippleRect() const;

	bool _active = false;
	::Ui::Animations::Simple _activeAnimation;
	::Ui::Animations::Simple _overAnimation;

};

} // namespace Dialogs
