fileName = fopen('token_1.txt','r');
A = fscanf(fileName,'%f');
fclose(fileName);

t = [1:1:length(A)]
figure(1);
plot(t,A);