package com.fsti.ladder.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.view.ContextMenu;
import android.widget.EditText;

public class InputEditText extends EditText {
    public InputEditText(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public InputEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public InputEditText(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub
    }

    @Override   
    protected   void  onCreateContextMenu(ContextMenu menu) {  
        //不做任何处理，为了阻止长按的时候弹出上下文菜单   
    }  

}
