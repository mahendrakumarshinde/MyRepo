package com.idragonit.bleexplorer;


public class UartChar {
    String RxChar;
    String Service;
    String TxChar;

    public UartChar(String s, String r, String t) {
        Service = s;
        RxChar = r;
        TxChar = t;
    }
}
