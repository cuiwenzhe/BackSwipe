package com.example.simplegestureinput;

import android.content.Context;
import android.view.LayoutInflater;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.example.simplegestureinput.util.GridMenu;

import java.util.List;

public class GridMenu6 implements GridMenu {
    static final int GRID_NUMBER = 6;
    int[] tileIds = {
            R.id.tile61, R.id.tile62, R.id.tile63, R.id.tile64,R.id.tile65, R.id.tile66,
    };
    FrameLayout frameLayout;
    LinearLayout suggestionLayout;
    Context context;
    boolean isShowing = false;
    int selectedTileIndex = -1;
    List<String> rawList;

    public GridMenu6(Context _context, FrameLayout _frameLayout){
        context = _context;
        frameLayout = _frameLayout;
        LayoutInflater inflater = LayoutInflater.from(context);
        suggestionLayout = (LinearLayout) inflater.inflate(R.layout.grid_menu_layout_6, null);
    }

    public void show(List<String> list){
        setupSuggestion(list);
        frameLayout.removeView(suggestionLayout);
        frameLayout.addView(suggestionLayout);
        isShowing = true;
    }

    public void clear(){
        clearTiles();
        frameLayout.removeView(suggestionLayout);
        isShowing = false;
    }
    public boolean isShowing(){
        return isShowing;
    }

    @Override
    public int getResultNumber() {
        return 7;
    }

    private void setupSuggestion(List<String> list){
        if(list.size() == 0){
            return;
        }
        rawList = list;
        selectedTileIndex = -1;
        clearTiles();

        int count = 0;
        for(String word : list.subList(1, list.size())){
            if(count >= GRID_NUMBER){
                break;
            }
            TextView tv = suggestionLayout.findViewById(tileIds[count++]);
            tv.setText(word);
        }

    }

    public String getHighLightWord(){
        if(selectedTileIndex == -1){
            return rawList.get(0);
        }
        return rawList.get(selectedTileIndex+1);
    }
    private void highlightTile(int index){
        int id = -1;
        if(index >= 0){
            id = tileIds[index % tileIds.length];
        }

        for(int idt : tileIds){
            TextView tv = suggestionLayout.findViewById(idt);
            tv.setBackground(context.getDrawable(R.drawable.tileback));
            if(idt == id){
                tv.setBackgroundColor(context.getColor(R.color.secondaryColor));
            }
        }
    }

    private int getTileIndexByDegree(int degree){
        float gridDegree = 360/GRID_NUMBER;
        if(degree == -1){ //-1 means a click
            return -1;
        }

        int index = (int)(degree/gridDegree);
        if(index < 3){
            index = 2 - index;
        }
        return index;
    }

    public void highlightSuggestionByDegree(int degree, int relativeDistance){
        selectedTileIndex = getTileIndexByDegree(degree);
        highlightTile(selectedTileIndex);
    }

    private void clearTiles(){
        for(int id : tileIds){
            TextView tv = suggestionLayout.findViewById(id);
            tv.setBackground(context.getDrawable(R.drawable.tileback));
            tv.setText("");
        }
    }
}
