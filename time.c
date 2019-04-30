
int main(){
	for (int i = 0; i < 1000; i++) {
		volatile unsigned long j;
		for ( j = 0; j < 1000000UL; j++) {
			;
		}
	}
	return 0;

}

