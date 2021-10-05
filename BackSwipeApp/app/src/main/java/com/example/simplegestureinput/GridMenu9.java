package com.example.simplegestureinput;

import android.content.Context;
import android.view.LayoutInflater;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.example.simplegestureinput.util.GridMenu;

import java.util.List;

public class GridMenu9 implements GridMenu {
    int[] tileIds = {
            R.id.tile12, R.id.tile02, R.id.tile01, R.id.tile00,
            R.id.tile10, R.id.tile20, R.id.tile21, R.id.tile22,
    };
    FrameLayout frameLayout;
    LinearLayout suggestionLayout;
    boolean isShowing = false;
    Context context;
    int selectedTileIndex = -1;
    List<String> rawList;

    public GridMenu9(Context _context, FrameLayout _frameLayout){
        context = _context;
        frameLayout = _frameLayout;
        LayoutInflater inflater = LayoutInflater.from(context);
        suggestionLayout = (LinearLayout) inflater.inflate(R.layout.grid_menu_layout_9, null);
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
        return 9;
    }

    private void setupSuggestion(List<String> list){
        if(list.size() == 0){
            return;
        }
        rawList = list;
        selectedTileIndex = -1;
        clearTiles();
        TextView firstTv = suggestionLayout.findViewById(R.id.tile11);
        firstTv.setText(list.get(0));
        firstTv.setBackgroundColor(context.getColor(R.color.secondaryColor));
        int count = 0;
        for(String word : list.subList(1, list.size())){
            if(count >= tileIds.length){
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
            TextView tv = suggestionLayout.findViewById(R.id.tile11);
            tv.setBackground(context.getDrawable(R.drawable.tileback));
        }
        if( id == -1){
            TextView tv = suggestionLayout.findViewById(R.id.tile11);
            tv.setBackgroundColor(context.getColor(R.color.secondaryColor));
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
        if(degree == -1){ //-1 means a click
            return -1;
        }
        degree = (int)((degree + 22.5) % 360);
        int index = (int)degree/45;
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
