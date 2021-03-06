/** 
 * @file lllayoutstack.h
 * @author Richard Nelson
 * @brief LLLayout class - dynamic stacking of UI elements
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Reshasearch, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLLAYOUTSTACK_H
#define LL_LLLAYOUTSTACK_H

#include "llpanel.h"


class LLLayoutPanel;


class LLLayoutStack : public LLView, public LLInstanceTracker<LLLayoutStack>
{
public:
	typedef enum e_layout_orientation
	{
		HORIZONTAL,
		VERTICAL
	} ELayoutOrientation;

	struct OrientationNames
	:	public LLInitParam::TypeValuesHelper<ELayoutOrientation, OrientationNames>
	{
		static void declareValues();
	};

	struct LayoutStackRegistry : public LLChildRegistry<LayoutStackRegistry>
	{};

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Mandatory<ELayoutOrientation, OrientationNames>	orientation;
		Optional<S32>			border_size;
		Optional<bool>			animate,
								clip;
		Optional<F32>			open_time_constant,
								close_time_constant;
		Optional<S32>			resize_bar_overlap;

		Params();
	};

	typedef LayoutStackRegistry child_registry_t;

	virtual ~LLLayoutStack();

	/*virtual*/ void draw();
	/*virtual*/ void removeChild(LLView*);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ bool addChild(LLView* child, S32 tab_group = 0);
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);


	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);

	typedef enum e_animate
	{
		NO_ANIMATE,
		ANIMATE
	} EAnimate;

	void addPanel(LLLayoutPanel* panel, EAnimate animate = NO_ANIMATE);
	void collapsePanel(LLPanel* panel, BOOL collapsed = TRUE);
	S32 getNumPanels() { return mPanels.size(); }

	void updateLayout();

	S32 getPanelSpacing() const { return mPanelSpacing; }
	
	static void updateClass();

protected:
	LLLayoutStack(const Params&);
	friend class LLUICtrlFactory;
	friend class LLLayoutPanel;

private:
	void updateResizeBarLimits();
	bool animatePanels();
	void createResizeBar(LLLayoutPanel* panel);

	const ELayoutOrientation mOrientation;

	typedef std::vector<LLLayoutPanel*> e_panel_list_t;
	e_panel_list_t mPanels;

	LLLayoutPanel* findEmbeddedPanel(LLPanel* panelp) const;
	LLLayoutPanel* findEmbeddedPanelByName(const std::string& name) const;
	void updateFractionalSizes();
	void updatePanelRect( LLLayoutPanel* param1, const LLRect& new_rect );

	S32 mPanelSpacing;

	// true if we already applied animation this frame
	bool mAnimatedThisFrame;
	bool mAnimate;
	bool mClip;
	F32  mOpenTimeConstant;
	F32  mCloseTimeConstant;
	bool mNeedsLayout;
	S32  mResizeBarOverlap;
}; // end class LLLayoutStack


class LLLayoutPanel : public LLPanel
{
friend class LLLayoutStack;
friend class LLUICtrlFactory;
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<S32>			expanded_min_dim,
								min_dim;
		Optional<bool>			user_resize,
								auto_resize;

		Params();
	};

	~LLLayoutPanel();

	void initFromParams(const Params& p);

	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	void handleReshape(const LLRect& new_rect, bool by_user);
	

	void setVisible(BOOL visible);

	S32 getLayoutDim() const;
	S32 getMinDim() const { return (mMinDim >= 0 || mAutoResize) ? llmax(0, mMinDim) : getLayoutDim(); }
	void setMinDim(S32 value) { mMinDim = value; }

	S32 getExpandedMinDim() const { return mExpandedMinDim >= 0 ? mExpandedMinDim : mMinDim; }
	void setExpandedMinDim(S32 value) { mExpandedMinDim = value; }
	
	S32 getRelevantMinDim() const
	{
		S32 min_dim = mMinDim;
		
		if (!mCollapsed)
		{
			min_dim = getExpandedMinDim();
		}
		
		return min_dim;
	}

	F32 getAutoResizeFactor() const;
	F32 getVisibleAmount() const;
	S32 getVisibleDim() const;

	void setOrientation(LLLayoutStack::ELayoutOrientation orientation);
	void storeOriginalDim();

	void setIgnoreReshape(bool ignore) { mIgnoreReshape = ignore; }
	
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLRect& rect)> reshape_signal_t;
	boost::signals2::connection setReshapeCallback(const reshape_signal_t::slot_type& cb);

protected:
	LLLayoutPanel(const Params& p);
	
	const bool	mAutoResize;
	const bool	mUserResize;

	S32		mExpandedMinDim;
	S32		mMinDim;
	bool	mCollapsed;
	F32		mVisibleAmt;
	F32		mCollapseAmt;
	F32		mFractionalSize;
	S32		mTargetDim;
	bool	mIgnoreReshape;
	LLLayoutStack::ELayoutOrientation mOrientation;
	class LLResizeBar* mResizeBar;
	
	reshape_signal_t*	mReshapeSignal;
	bool				mReshapeSignalFire;
};


#endif
