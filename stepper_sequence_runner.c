/*
 * PIC16F877A Stepper Sequence Runner v3
 * 20MHz | 28BYJ-48 + ULN2003A | SSD1306 OLED | 4x4 Keypad
 * RD0-RD3=Coils  RB0-RB3=KpadCols  RB4-RB7=KpadRows
 * RC3=SCL  RC4=SDA
 *
 * KEYS (main menu):
 *  1=Angle  2=Rotations  3=Cycle  4=Timed
 *  5=View   6=Load       #=RUN    *=Clear
 *
 * CYCLE STEP (key 3):
 *  - ROT TIME : motor ON  for N seconds (1-99)
 *  - IDLE TIME: motor OFF for N seconds (0-99, 0=no pause)
 *  - Repeats until * pressed (within sequence run)
 *
 * RUN (#):
 *  - Asks: 1=LOOP FOREVER  2=COUNT
 *  - COUNT: enter how many times to repeat full sequence
 */
#include <xc.h>
#include <stdint.h>
#pragma config FOSC=HS,WDTE=OFF,PWRTE=ON,BOREN=ON,LVP=OFF,CP=OFF,CPD=OFF,WRT=OFF
#define _XTAL_FREQ 20000000UL
#define SPR 4096U

static char ks(void);
static char kw(void);

/* ===== MOTOR ===== */
static const uint8_t HS[8]={0x09,0x01,0x03,0x02,0x06,0x04,0x0C,0x08};
static volatile uint32_t vl=0;
static volatile uint8_t  vi=0,vcw=1;
static volatile uint16_t vr=0;

void __interrupt() isr(void){
    if(!PIR1bits.TMR1IF)return;
    PIR1bits.TMR1IF=0;
    TMR1H=(uint8_t)(vr>>8);TMR1L=(uint8_t)vr;
    if(vl){
        PORTD=(PORTD&0xF0)|HS[vi];
        vi=vcw?(vi+1)&7:(vi+7)&7;
        if(!--vl){PORTD&=0xF0;T1CONbits.TMR1ON=0;}
    }else{PORTD&=0xF0;T1CONbits.TMR1ON=0;}
}
static void mg(uint32_t s,uint8_t cw,uint8_t rpm){
    uint32_t t=37500000UL/((uint32_t)SPR*rpm);
    if(t>65534)t=65534;
    vr=(uint16_t)(65536UL-t);vcw=cw;vl=s;
    TMR1H=(uint8_t)(vr>>8);TMR1L=(uint8_t)vr;
    T1CONbits.TMR1ON=1;
}
static void ms(void){T1CONbits.TMR1ON=0;vl=0;PORTD&=0xF0;}

/* wait for motor to finish, returns 1 if * pressed */
static uint8_t mw(void){
    __delay_ms(150);
    while(vl){if(ks()=='*'){ms();return 1;}}
    ms();__delay_ms(200);return 0;
}

/* wait N seconds, returns 1 if * pressed */
static uint8_t wsec(uint8_t s){
    while(s--){
        __delay_ms(500);if(ks()=='*')return 1;
        __delay_ms(500);if(ks()=='*')return 1;
    }
    return 0;
}

/* ===== I2C ===== */
static void scl(uint8_t v){TRISCbits.TRISC3=0;PORTCbits.RC3=v;}
static void sda(uint8_t v){TRISCbits.TRISC4=0;PORTCbits.RC4=v;}
static void sdai(void){TRISCbits.TRISC4=1;}
static void sdao(void){TRISCbits.TRISC4=0;}
#define DY() __delay_us(5)
static void i2s(void){sda(1);DY();scl(1);DY();sda(0);DY();scl(0);DY();}
static void i2p(void){sda(0);DY();scl(1);DY();sda(1);DY();}
static void i2b(uint8_t d){
    uint8_t i;
    for(i=0;i<8;i++){sda((d&0x80)?1:0);DY();scl(1);DY();scl(0);DY();d<<=1;}
    sdai();DY();scl(1);DY();
    {uint8_t t=200;while(PORTCbits.RC4&&--t);}
    scl(0);DY();sdao();
}

/* ===== OLED ===== */
#define OA 0x78
static void oc(uint8_t c){i2s();i2b(OA);i2b(0);i2b(c);i2p();}
static void opos(uint8_t col,uint8_t pg){oc(0xB0|pg);oc(col&0xF);oc(0x10|(col>>4));}
static void opg(uint8_t pg,uint8_t v){
    uint8_t i;opos(0,pg);i2s();i2b(OA);i2b(0x40);
    for(i=0;i<128;i++)i2b(v);i2p();
}
static void oclr(void){uint8_t p;for(p=0;p<8;p++)opg(p,0);}
static void oinit(void){
    __delay_ms(300);
    oc(0xAE);oc(0xD5);oc(0x80);oc(0xA8);oc(0x3F);
    oc(0xD3);oc(0x00);oc(0x40);oc(0x8D);oc(0x14);
    oc(0x20);oc(0x02);oc(0xA1);oc(0xC8);
    oc(0xDA);oc(0x12);oc(0x81);oc(0xCF);
    oc(0xD9);oc(0xF1);oc(0xDB);oc(0x40);
    oc(0xA4);oc(0xA6);oc(0x2E);oclr();oc(0xAF);
}

/* ===== FONT ASCII 32-90 ===== */
static const uint8_t F[][5]={
{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
{0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
{0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},
{0x00,0x41,0x22,0x1C,0x00},{0x08,0x2A,0x1C,0x2A,0x08},{0x08,0x08,0x3E,0x08,0x08},
{0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
{0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},
{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
{0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},
{0x00,0x56,0x36,0x00,0x00},{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},
{0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},
{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
{0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
{0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
{0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
{0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
};
static void och(uint8_t col,uint8_t pg,char c,uint8_t inv){
    uint8_t b[6],i,x;
    if(c<32||c>90)c=32;x=(uint8_t)(c-32);
    for(i=0;i<5;i++)b[i]=inv?(uint8_t)(~F[x][i]):F[x][i];
    b[5]=inv?0xFF:0;
    opos(col,pg);i2s();i2b(OA);i2b(0x40);
    for(i=0;i<6;i++)i2b(b[i]);i2p();
}
static void os(uint8_t col,uint8_t pg,const char*s,uint8_t inv){
    while(*s&&col+6<=128){och(col,pg,*s++,inv);col+=6;}
}
static void on(uint8_t col,uint8_t pg,uint32_t v,uint8_t w){
    char b[7];uint8_t i=w;b[i]=0;
    do{b[--i]=(char)('0'+v%10);v/=10;}while(v&&i);
    while(i)b[--i]=' ';os(col,pg,b,0);
}
static void otit(const char*s){opg(0,0xFF);os(2,0,s,1);}

/* ===== KEYPAD ===== */
static const char KM[4][4]={
    {'1','2','3','A'},{'4','5','6','B'},
    {'7','8','9','C'},{'*','0','#','D'}
};
static void kinit(void){TRISB=0x0F;OPTION_REGbits.nRBPU=0;PORTB=0xF0;}
static char ks(void){
    uint8_t r,c;
    for(r=0;r<4;r++){
        PORTB=0xF0&~(0x10<<r);__delay_us(50);
        for(c=0;c<4;c++){
            if(!(PORTB&(1<<c))){__delay_ms(15);
                if(!(PORTB&(1<<c))){
                    while(!(PORTB&(1<<c)));
                    __delay_ms(15);PORTB=0xF0;return KM[r][c];
                }
            }
        }
    }
    PORTB=0xF0;return 0;
}
static char kw(void){char k;do{k=ks();}while(!k);return k;}

/* ===== NUMBER INPUT ===== */
static uint32_t gn(uint8_t col,uint8_t pg,uint32_t mx){
    char b[6];uint8_t n=0,i;char k;b[0]=0;
    while(1){
        for(i=0;i<n;i++)och((uint8_t)(col+i*6),pg,b[i],0);
        opos((uint8_t)(col+n*6),pg);i2s();i2b(OA);i2b(0x40);
        for(i=0;i<6;i++)i2b(0x3E);i2p();
        for(i=(uint8_t)(n+1);i<=5;i++){
            uint8_t j;opos((uint8_t)(col+i*6),pg);
            i2s();i2b(OA);i2b(0x40);
            for(j=0;j<6;j++)i2b(0);i2p();
        }
        k=kw();
        if(k>='0'&&k<='9'){if(n<5){b[n++]=k;b[n]=0;}}
        else if(k=='*'){if(n)b[--n]=0;else return 0xFFFFFFFF;}
        else if(k=='#'){
            if(!n)return 0xFFFFFFFF;
            uint32_t v=0;
            for(i=0;i<n;i++)v=v*10+(uint32_t)(b[i]-'0');
            /* allow 0 only if mx allows it (for idle time) */
            if(v>mx){os(col,pg,"ERR  ",0);__delay_ms(400);opg(pg,0);n=0;b[0]=0;}
            else return v;
        }
    }
}
/* gn that allows 0 */
static uint32_t gn0(uint8_t col,uint8_t pg,uint32_t mx){
    char b[6];uint8_t n=0,i;char k;b[0]=0;
    while(1){
        for(i=0;i<n;i++)och((uint8_t)(col+i*6),pg,b[i],0);
        opos((uint8_t)(col+n*6),pg);i2s();i2b(OA);i2b(0x40);
        for(i=0;i<6;i++)i2b(0x3E);i2p();
        for(i=(uint8_t)(n+1);i<=5;i++){
            uint8_t j;opos((uint8_t)(col+i*6),pg);
            i2s();i2b(OA);i2b(0x40);
            for(j=0;j<6;j++)i2b(0);i2p();
        }
        k=kw();
        if(k>='0'&&k<='9'){if(n<5){b[n++]=k;b[n]=0;}}
        else if(k=='*'){if(n)b[--n]=0;else return 0xFFFFFFFF;}
        else if(k=='#'){
            if(!n)return 0xFFFFFFFF;
            uint32_t v=0;
            for(i=0;i<n;i++)v=v*10+(uint32_t)(b[i]-'0');
            if(v>mx){os(col,pg,"ERR  ",0);__delay_ms(400);opg(pg,0);n=0;b[0]=0;}
            else return v;
        }
    }
}

static uint8_t gdir(void){
    os(0,5,"1-CW   2-CCW",0);opg(6,0);
    while(1){
        char k=kw();
        if(k=='1'){os(0,6,">> CW",0);__delay_ms(300);return 1;}
        if(k=='2'){os(0,6,">> CCW",0);__delay_ms(300);return 0;}
        if(k=='*')return 0xFF;
    }
}

/* ===== SEQUENCE ===== */
#define MAXS 5
/*
 * Step types:
 *  0 = ANGLE   : val=degrees (1-3600)
 *  1 = ROT     : val=rotations (1-99)
 *  2 = CYCLE   : val=rot_sec (1-99), val2=idle_sec (0-99)
 *  3 = TIMED   : val=seconds (1-99)
 */
typedef struct{uint8_t t;uint32_t val;uint8_t val2;uint8_t cw;uint8_t rpm;}St;
static St sq[MAXS],sv[MAXS];
static uint8_t sc=0,svc=0;
static const char TN[4]={'A','R','C','T'};

static void cps(St*d,St*s){
    d->t=s->t;d->val=s->val;d->val2=s->val2;d->cw=s->cw;d->rpm=s->rpm;
}

/* ===== MAIN MENU ===== */
static void smenu(void){
    oclr();otit("SEQ RUNNER");
    os(0,2,"1-ANG  2-ROT",0);
    os(0,3,"3-CYC  4-TIM",0);
    os(0,4,"5-VIEW 6-LOAD",0);
    opg(5,0xFF);
    os(0,6,"#-RUN  *-CLEAR",0);
    os(0,7,"STEPS:",0);on(42,7,sc,1);os(54,7,"OF 5",0);
}

/* ===== VIEW STEPS ===== */
static void view_steps(void){
    if(!sc){
        oclr();otit("VIEW STEPS");
        os(0,3,"NO STEPS YET",0);
        __delay_ms(1200);return;
    }
    uint8_t idx=0;
    while(1){
        St*p=&sq[idx];
        oclr();
        opg(0,0xFF);os(2,0,"1-PRV 2-NXT *-BCK",1);
        os(0,2,"STEP:",0);on(36,2,idx+1,1);os(48,2,"OF",0);on(66,2,sc,1);
        opg(3,0xFF);
        switch(p->t){
            case 0:os(0,4,"ANGLE:",0);on(42,4,p->val,4);os(66,4,"DEG",0);break;
            case 1:os(0,4,"ROT:",0);on(30,4,p->val,2);os(48,4,"TURNS",0);break;
            case 2:
                os(0,4,"CYCLE",0);
                os(0,5,"ON:",0);on(24,5,p->val,2);os(42,5,"S",0);
                os(54,5,"OFF:",0);on(84,5,p->val2,2);os(102,5,"S",0);
                break;
            case 3:os(0,4,"TIMED:",0);on(42,4,p->val,2);os(60,4,"SEC",0);break;
        }
        if(p->t!=2){
            os(0,5,p->cw?"DIR: CW":"DIR: CCW",0);
        }
        os(0,6,p->cw?"DIR: CW":"DIR: CCW",0);
        on(0,7,p->rpm,2);os(14,7,"RPM",0);
        char k=kw();
        if(k=='2'&&idx<sc-1)idx++;
        else if(k=='1'&&idx>0)idx--;
        else if(k=='*')return;
    }
}

/* ===== ADD STEP ===== */
static void addst(uint8_t type){
    uint32_t val=0,val2=0;uint8_t dir=1;uint32_t rpm;

    oclr();otit("ADD STEP");
    switch(type){
        case 0:
            os(0,2,"TYPE: ANGLE",0);
            os(0,3,"1-3600 DEG",0);
            opg(4,0xFF);os(0,5,"DEG:",0);
            val=gn(28,5,3600);break;
        case 1:
            os(0,2,"TYPE: ROTATIONS",0);
            os(0,3,"1-99 TURNS",0);
            opg(4,0xFF);os(0,5,"ROT:",0);
            val=gn(28,5,99);break;
        case 2:
            /* CYCLE: rot time then idle time */
            os(0,2,"TYPE: CYCLE",0);
            os(0,3,"ROT TIME 1-99S",0);
            opg(4,0xFF);os(0,5,"ON SEC:",0);
            val=gn(48,5,99);
            if(val==0xFFFFFFFF)return;
            /* idle time - allow 0 */
            oclr();otit("ADD STEP");
            os(0,2,"TYPE: CYCLE",0);
            os(0,3,"IDLE TIME 0-99S",0);
            os(0,4,"(0=NO PAUSE)",0);
            opg(5,0xFF);os(0,6,"OFF SEC:",0);
            val2=gn0(54,6,99);
            if(val2==0xFFFFFFFF)return;
            break;
        case 3:
            os(0,2,"TYPE: TIMED",0);
            os(0,3,"1-99 SECONDS",0);
            opg(4,0xFF);os(0,5,"SEC:",0);
            val=gn(28,5,99);break;
    }
    if(val==0xFFFFFFFF)return;

    /* direction */
    oclr();otit("ADD STEP");
    os(0,2,"SET DIRECTION",0);opg(3,0xFF);os(0,4,"CHOOSE:",0);
    dir=gdir();if(dir==0xFF)return;

    /* rpm */
    oclr();otit("ADD STEP");
    os(0,2,"SET SPEED",0);os(0,3,"1-15 RPM",0);
    opg(4,0xFF);os(0,5,"# CONFIRM",0);os(0,6,"RPM:",0);
    rpm=gn(28,6,15);if(rpm==0xFFFFFFFF)return;

    /* save */
    sq[sc].t=type;sq[sc].val=val;sq[sc].val2=(uint8_t)val2;
    sq[sc].cw=dir;sq[sc].rpm=(uint8_t)rpm;sc++;

    /* confirm */
    oclr();otit("STEP SAVED");
    os(0,2,"STEP:",0);on(36,2,sc,1);os(48,2,"OF 5",0);
    opg(3,0xFF);
    switch(type){
        case 0:os(0,4,"ANGLE:",0);on(42,4,val,4);os(66,4,"DEG",0);break;
        case 1:os(0,4,"ROT:",0);on(30,4,val,2);os(48,4,"TURNS",0);break;
        case 2:
            os(0,4,"ON:",0);on(24,4,val,2);os(42,4,"S",0);
            os(54,4,"OFF:",0);on(84,4,val2,2);os(102,4,"S",0);break;
        case 3:os(0,4,"TIMED:",0);on(42,4,val,2);os(60,4,"SEC",0);break;
    }
    os(0,5,dir?"DIR: CW":"DIR: CCW",0);
    os(0,6,"RPM:",0);on(30,6,rpm,2);
    os(0,7,"ANY KEY=NEXT",0);
    kw();
}

/* ===== RUN ONE STEP, returns 1 if aborted ===== */
static uint8_t run_step(St*p){
    uint32_t steps;
    if(p->t==0){
        /* ANGLE */
        steps=(p->val*(uint32_t)SPR)/360;if(!steps)steps=1;
        mg(steps,p->cw,p->rpm);
        return mw();
    }else if(p->t==1){
        /* ROTATIONS */
        steps=p->val*(uint32_t)SPR;
        mg(steps,p->cw,p->rpm);
        return mw();
    }else if(p->t==2){
        /* CYCLE: ON/OFF until * */
        os(0,7,"* = STOP",0);
        while(1){
            /* ON phase */
            opg(6,0);os(0,6,"ON ",0);on(24,6,p->val,2);os(42,6,"S",0);
            mg(0xFFFFFFFF,p->cw,p->rpm);
            if(wsec((uint8_t)p->val)){ms();return 1;}
            ms();
            /* IDLE phase (skip if 0) */
            if(p->val2>0){
                opg(6,0);os(0,6,"IDLE",0);on(30,6,p->val2,2);os(48,6,"S",0);
                if(wsec(p->val2))return 1;
            }
        }
    }else{
        /* TIMED */
        uint8_t s=(uint8_t)p->val;
        mg(0xFFFFFFFF,p->cw,p->rpm);
        while(s>0){
            opg(7,0);os(0,7,"SEC:",0);on(30,7,s,2);
            __delay_ms(500);if(ks()=='*'){ms();return 1;}
            __delay_ms(500);if(ks()=='*'){ms();return 1;}
            s--;
        }
        ms();return 0;
    }
    return 0;
}

/* ===== RUN SEQUENCE ===== */
static void runseq(void){
    if(!sc){
        oclr();otit("ERROR");
        os(0,3,"NO STEPS!",0);
        os(0,4,"ADD STEPS FIRST",0);
        __delay_ms(1400);return;
    }

    /* Ask: loop forever or count */
    oclr();otit("RUN MODE");
    os(0,2,"1 - LOOP",0);
    os(0,3,"    UNTIL *",0);
    os(0,4,"2 - COUNT",0);
    os(0,5,"    N TIMES",0);
    opg(6,0xFF);
    os(0,7,"CHOOSE 1 OR 2",0);

    uint8_t loop_forever=0;
    uint32_t count=1;
    while(1){
        char k=kw();
        if(k=='1'){loop_forever=1;break;}
        if(k=='2'){
            oclr();otit("RUN COUNT");
            os(0,2,"HOW MANY TIMES?",0);
            os(0,3,"1-99",0);
            opg(4,0xFF);os(0,5,"COUNT:",0);
            count=gn(42,5,99);
            if(count==0xFFFFFFFF)return;
            break;
        }
        if(k=='*')return;
    }

    __delay_ms(200);while(ks());

    uint32_t iter=0;
    uint8_t ab=0;

    while(!ab){
        iter++;
        uint8_t i;
        for(i=0;i<sc&&!ab;i++){
            /* show running screen */
            oclr();otit("RUNNING");
            if(loop_forever){
                os(0,2,"LOOP:",0);on(36,2,iter,3);os(60,2,"* STOP",0);
            }else{
                os(0,2,"RUN:",0);on(30,2,iter,2);os(48,2,"OF",0);on(66,2,count,2);
            }
            os(0,3,"STEP:",0);on(36,3,i+1,1);os(48,3,"OF",0);on(66,3,sc,1);
            opg(4,0xFF);
            St*p=&sq[i];
            switch(p->t){
                case 0:os(0,5,"ANG:",0);on(30,5,p->val,4);os(60,5,"DEG",0);break;
                case 1:os(0,5,"ROT:",0);on(30,5,p->val,2);os(48,5,"TURNS",0);break;
                case 2:os(0,5,"CYC ON:",0);on(54,5,p->val,2);os(72,5,"S",0);break;
                case 3:os(0,5,"TIM:",0);on(30,5,p->val,2);os(48,5,"SEC",0);break;
            }
            os(0,6,p->cw?"CW":"CCW",0);os(24,6,"RPM:",0);on(54,6,p->rpm,2);
            ab=run_step(p);
            if(!ab){opg(7,0);os(0,7,"STEP OK",0);__delay_ms(300);}
        }
        /* check if count reached */
        if(!loop_forever&&iter>=count)break;
        /* check * between iterations */
        if(!ab&&ks()=='*')break;
    }

    ms();

    /* result */
    oclr();otit("DONE");
    if(ab){
        os(0,3,"STOPPED",0);
    }else{
        os(0,3,"COMPLETE",0);
        if(!loop_forever){os(0,4,"RUNS:",0);on(36,4,iter,2);}
    }
    opg(5,0xFF);
    os(0,6,"SAVE? 1=YES 2=NO",0);
    while(1){
        char k=kw();
        if(k=='1'){
            uint8_t j;
            for(j=0;j<sc;j++)cps(&sv[j],&sq[j]);
            svc=sc;sc=0;
            opg(7,0);os(0,7,"SAVED! 6=LOAD",0);
            __delay_ms(1200);break;
        }
        if(k=='2'){sc=0;opg(7,0);os(0,7,"CLEARED",0);__delay_ms(800);break;}
    }
}

/* ===== LOAD ===== */
static void loadseq(void){
    oclr();otit("LOAD SAVED");
    if(!svc){os(0,3,"NOTHING SAVED",0);__delay_ms(1200);return;}
    uint8_t i;for(i=0;i<svc;i++)cps(&sq[i],&sv[i]);sc=svc;
    os(0,3,"LOADED:",0);on(48,3,sc,1);os(60,3,"STEPS",0);
    os(0,5,"# TO RUN",0);
    __delay_ms(1200);
}

/* ===== MAIN ===== */
void main(void){
    ADCON1=0x07;TRISA=0;TRISD=0;PORTD=0;TRISC=0xFF;
    TRISCbits.TRISC3=0;PORTCbits.RC3=1;
    TRISCbits.TRISC4=0;PORTCbits.RC4=1;
    TRISD&=0xF0;PORTD&=0xF0;
    T1CON=0x30;TMR1H=TMR1L=0;
    PIR1bits.TMR1IF=0;PIE1bits.TMR1IE=1;
    INTCONbits.PEIE=1;INTCONbits.GIE=1;
    kinit();__delay_ms(10);oinit();
    oclr();otit("SEQ RUNNER V3");
    os(4,3,"PIC16F877A",0);os(4,5,"28BYJ-48",0);
    __delay_ms(1200);
    while(1){
        smenu();
        char k=kw();
        if     (k=='1'&&sc<MAXS)addst(0);
        else if(k=='2'&&sc<MAXS)addst(1);
        else if(k=='3'&&sc<MAXS)addst(2);
        else if(k=='4'&&sc<MAXS)addst(3);
        else if(k=='5')view_steps();
        else if(k=='6')loadseq();
        else if(k=='#')runseq();
        else if(k=='*'){sc=0;oclr();otit("CLEARED");__delay_ms(700);}
        else if(sc>=MAXS){oclr();otit("FULL");os(0,3,"#=RUN *=CLR",0);__delay_ms(1000);}
    }
}