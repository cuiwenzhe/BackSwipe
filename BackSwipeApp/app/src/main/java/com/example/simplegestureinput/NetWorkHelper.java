package com.example.simplegestureinput;

import android.content.Context;
import android.net.DhcpInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.util.Log;

import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;

import org.apache.commons.io.IOUtils;

import java.io.IOException;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

public class NetWorkHelper {
    Context mContext;

    String TAG = NetWorkHelper.class.getSimpleName();
    int BROADCAST_PORT = 9527;
    int RECEIVER_PORT = 9528;
    String sniffingMessage = "RequestServerForGesture:" + RECEIVER_PORT;
    DatagramSocket broadcastSocket;
    ServerSocket serverSocket = null;
    NetWorkResponseInterface responseInterface;

    String gestureHost = "";
    int gesturePort = -1;

    public void setHostPort(String host, int port){
        gestureHost = host;
        gesturePort = port;
    }
    public NetWorkHelper(Context _context, NetWorkResponseInterface _responseInterface){
        responseInterface = _responseInterface;
        mContext = _context;
        try {
            broadcastSocket = new DatagramSocket();
            broadcastSocket.setBroadcast(true);
            serverSocket = new ServerSocket(RECEIVER_PORT);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    public void finish(){
        if(serverSocket!= null){
            try {
                serverSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private class SendConfirmBytes extends AsyncTask<byte[], String, String>{

        @Override
        protected String doInBackground(byte[]... bytes) {
            if(gestureHost.equals("") || gesturePort == -1){
                Log.i(TAG, "Fail to find Gesture Server");
                return null;
            }
            Socket client = null;
            try {
                client = new Socket(gestureHost, gesturePort);
                client.setSoTimeout(5000);
                OutputStream ots = client.getOutputStream();
                Log.i(TAG, "The lenght of bytes is " + bytes[0].length);
                ots.write(bytes[0]);
                ots.flush();
                ots.close();
                client.close();

            } catch (IOException e) {
                e.printStackTrace();
                Log.i(TAG, e.getMessage());
                try {
                    client.close();
                } catch (IOException ex) {
                    ex.printStackTrace();
                }
            }
            return null;
        }
    }
    private class SendDecodeBytes extends AsyncTask<byte[], String, String>{

        @Override
        protected String doInBackground(byte[]... bytes) {
            if(gestureHost.equals("") || gesturePort == -1){
                Log.i(TAG, "Fail to find Gesture Server");
                return null;
            }
            Socket client = null;
            try {
                client = new Socket(gestureHost, gesturePort);
                client.setSoTimeout(5000);
                OutputStream ots = client.getOutputStream();
                Log.i(TAG, "The lenght of bytes is " + bytes[0].length);
                ots.write(bytes[0]);
                ots.flush();
                ots.close();
                client.close();

                if(serverSocket != null){
                    try {
                        if(serverSocket.isClosed()){
                            serverSocket = new ServerSocket(RECEIVER_PORT);
                        }
                        serverSocket.setSoTimeout(5000);
                        Socket socket = serverSocket.accept();
                        String result = IOUtils.toString(socket.getInputStream(), StandardCharsets.UTF_8.name());
                        publishProgress(getStringById(R.string.decode_finish),result);
                        //serverSocket.close();
                        Log.i(TAG, result);
                    } catch (IOException e) {
                        e.printStackTrace();
                        if(e.getClass() == SocketTimeoutException.class){
                            Log.i(TAG, "Timeout");
                            publishProgress("Server Timeout");
                        }
                    }
                }

            } catch (IOException e) {
                e.printStackTrace();
                Log.i(TAG, e.getMessage());
                try {
                    client.close();
                } catch (IOException ex) {
                    ex.printStackTrace();
                }
            }
            return null;
        }
        @Override
        protected void onProgressUpdate(String... msg){
            if(msg[0].equals(getStringById(R.string.decode_finish))){
                JSONObject resJSON = JSON.parseObject(msg[1]);
                List<String> decodeWords = JSON.parseArray(resJSON.getJSONArray("RESULT_WORDS").toJSONString(),String.class);
                List<Double> decodeScores = JSON.parseArray(resJSON.getJSONArray("RESULT_SCORES").toJSONString(),Double.class);

                responseInterface.setDecodingResults(decodeWords, decodeScores);



            }
        }
    }
    /**
     * This function helps to find the server IP and Port by broadcast message
     * in WLAN. By default it wait for the response in 2 second.
     */
    private class SendBroadcast extends AsyncTask<String, String, String>{

        @Override
        protected String doInBackground(String... strings) {
            InetAddress broadcast = null;
            publishProgress("Sending Sniffing Message");
            try {
                broadcast = getBroadcastAddress();
            } catch (IOException e) {
                e.printStackTrace();
            }
            if(broadcast == null){
                return "fail";
            }

            try {
                byte[] data = sniffingMessage.getBytes();
                DatagramPacket sendPacket = new DatagramPacket(data,
                        data.length, broadcast, BROADCAST_PORT);
                broadcastSocket.send(sendPacket);
                Log.d(TAG, getClass().getName() + ">>> Request packet sent to: " +
                        broadcast.getHostAddress());

            } catch (Exception e) {
                e.printStackTrace();
            }

            publishProgress("Waiting for server response");
            if(serverSocket != null){
                try {
                    if(serverSocket.isClosed()){
                        serverSocket = new ServerSocket(RECEIVER_PORT);
                    }
                    serverSocket.setSoTimeout(5000);
                    Socket socket = serverSocket.accept();
                    String result = IOUtils.toString(socket.getInputStream(), StandardCharsets.UTF_8.name());
                    publishProgress(result);
                    serverSocket.close();
                    String[] strs = result.split(":");
                    gestureHost = strs[0];
                    gesturePort = Integer.parseInt(strs[1]);
                    Log.i(TAG, result);
                } catch (IOException e) {
                    e.printStackTrace();
                    if(e.getClass() == SocketTimeoutException.class){
                        Log.i(TAG, "Timeout");
                        publishProgress("Server Timeout");
                    }
                }
            }
            return "complete";
        }
        @Override
        protected void onProgressUpdate(String... msg){
            responseInterface.onResponseReceived(msg[0]);
        }
    }
    public void sniffHost(){
        //new ListeningServer().execute("","");
        if(serverSocket.isClosed()){
            try {
                serverSocket = new ServerSocket(RECEIVER_PORT);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        new SendBroadcast().execute(sniffingMessage);
    }

    public void sendGesture(byte[] msg){
        new SendDecodeBytes().execute(msg);
    }

    public void sendTaskMessage(byte[] msg){
        new SendConfirmBytes().execute(msg);
    }


    InetAddress getBroadcastAddress() throws IOException {
        WifiManager wifi = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
        DhcpInfo dhcp = wifi.getDhcpInfo();
        // handle null somehow

        int broadcast = (dhcp.ipAddress & dhcp.netmask) | ~dhcp.netmask;
        byte[] quads = new byte[4];
        for (int k = 0; k < 4; k++)
            quads[k] = (byte) ((broadcast >> k * 8) & 0xFF);
        return InetAddress.getByAddress(quads);
    }

    private String getStringById(int id){
        return mContext.getResources().getString(id);
    }
}