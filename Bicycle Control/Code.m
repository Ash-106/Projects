prompt1='velocity ';
syms s v t;
format longg;
v=input(prompt1);
A=[0 0 1 0;
     0 0 0 1; 
     13.67 0.225-1.319*v^2 -0.164*v -0.552*v;
     4.857 10.81-1.125*v^2 3.621*v -2.388*v];
B=[0; 0; -0.339; 7.457];
C=[1 0 0 0];
D=[0];
[V,d]=eig(A)

I=eye(4);
tfun=C*(inv((s*I-A)))*B+D;
display(vpa(simplify(tfun)));
[b,a]=ss2tf(A,B,C,D)       %state space to tranfer func
[z,p] = tf2zp(b,a)          %transfer func to poles zeroes
sys = tf((b),(a))  

x_0=[5;3;7;5];
x(t)=V*expm(d*t)/V*x_0; %trying to plot the zero input response, cant find a command to do it directly%
y(t)=C*x(t);
figure('Name', 'Zero Input Response');
fplot(y(t),t)

figure('Name', 'Nyquist Plot');
nyquistplot(sys);

figure('Name', 'Root Locus');
rlocus(sys);

figure('Name', 'Bode Plot');
bode((sys))

sisotool(sys)