class Programms{
    public:
        //для экономии памяти будем записывать опции включения программы в разные биты одной переменной - enabling_options, где:
        //1-7 биты - дни недели(1-воскресенье, 2-понедельник, 3-вторник, 4-среда, 5-четверг, 6-пятница, 7-суббота)
        //0 бит - включена или выключена программа
        int enabling_options;      //опции включения
        int hour_on;               //час включения
        int min_on;                //минута включения
        int hour_off;              //час выключения
        int min_off;               //минута выключения
        //Конструктор класса, в котором обнуляем поля при создании объекта
        Programms(){
            hour_on=0;
            min_on=0;
            hour_off=0;
            min_off=0;
            enabling_options=0;
        }
        //метод получения состояния активности программы в конкретный день недели (допустимые значения 1-7)
        bool get_weekday(int num_weekday){
            if (enabling_options & (1<<num_weekday))
                return true;
            else 
                return false;
        }
        //метод получения состояния активности программы в целом
        bool get_activ_prog(){
            if (enabling_options & (1<<0))
                return true;
            else
                return false;
        }
        /*//метод устанавки времени включения и выключения
        void set(int h_on, int m_on, int h_off, int m_off, int en_opt){
            hour_on=h_on;
            min_on=m_on;
            hour_off=h_off;
            min_off=m_off;
            enabling_options=en_opt;
        }
        //метод установки опций включения программы 
        void set(int num_options, bool activity){
            if (activity){
                enabling_options |= (1 << num_options);   // x |= (1 << n);Чтобы записать единицу в бит n, числа x
            }
            else{
                enabling_options &= ~(1 << num_options);  //x &= ~(1 << n);Чтобы записать ноль в бит n, числа x
            }
        }
        void set(int en_opt){
            enabling_options=en_opt;
        }*/
};