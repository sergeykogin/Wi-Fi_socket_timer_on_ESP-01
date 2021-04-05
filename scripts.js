//получение значения бита в поле options
		function getBitInOptions(num_bit, options){
			if(options & (1<<num_bit))
				return true;
			else
				return false;
		}
		//получаем с сервера JSON и заполняем загруженную страницу
		function getProgramms(text){
			var obj = JSON.parse(text);																	//разбираем строку в формате JSON (преобразование строки в объект JS)
			var num_prog=0;
			for (iter in obj) {																			//перебираем программы и заполняем поля страницы 
				document.getElementById("hour_on"+num_prog).value=obj[iter].hour_on;					//заполнение поля "час включения"
				document.getElementById("min_on"+num_prog).value=obj[iter].min_on;						//заполнение поля "минута включения"
				document.getElementById("hour_off"+num_prog).value=obj[iter].hour_off;					//заполнение поля "час выключения"
				document.getElementById("min_off"+num_prog).value=obj[iter].min_off;					//заполнение поля "минута выключения"
				document.getElementById("flag_on"+num_prog).checked=getBitInOptions(0, obj[iter].options);// заполняем чекбокс "on/off"
				document.getElementById("su"+num_prog).checked=getBitInOptions(1, obj[iter].options);	// заполняем чекбокс "воскресенье"
				document.getElementById("mon"+num_prog).checked=getBitInOptions(2, obj[iter].options);	// заполняем чекбокс "понедельник"
				document.getElementById("tu"+num_prog).checked=getBitInOptions(3, obj[iter].options);	// ...
				document.getElementById("we"+num_prog).checked=getBitInOptions(4, obj[iter].options);
				document.getElementById("thu"+num_prog).checked=getBitInOptions(5, obj[iter].options);
				document.getElementById("fr"+num_prog).checked=getBitInOptions(6, obj[iter].options);
				document.getElementById("sat"+num_prog).checked=getBitInOptions(7, obj[iter].options);
				
				num_prog++;
			}
		}
		//функция проверяет чекбокс (options), если он true, записывает в бит (num_bit) единицу, иначе ноль
		function formOptions(num_bit, options){
			let result;
				if(options)
					result |= (1 << num_bit);
				else
					result &= ~(1 << num_bit);
			return result;
		}
		//собираем данные со страницы и формируем JSON (возвращаем строку)
		function formJSON(){
			var resJSON="{";
			let range=5;
			for(let i=0;i<range;i++){
				let buf=0;
				resJSON+='"program'+i+'": {';
				resJSON+='"hour_on"'+":"+document.getElementById("hour_on"+i).value+", ";
				resJSON+='"min_on"'+":"+document.getElementById("min_on"+i).value+", ";
				resJSON+='"hour_off"'+":"+document.getElementById("hour_off"+i).value+", ";
				resJSON+='"min_off"'+":"+document.getElementById("min_off"+i).value+", ";
				buf+=formOptions(0,document.getElementById("flag_on"+i).checked);
				buf+=formOptions(1,document.getElementById("su"+i).checked);
				buf+=formOptions(2,document.getElementById("mon"+i).checked);
				buf+=formOptions(3,document.getElementById("tu"+i).checked);
				buf+=formOptions(4,document.getElementById("we"+i).checked);
				buf+=formOptions(5,document.getElementById("thu"+i).checked);
				buf+=formOptions(6,document.getElementById("fr"+i).checked);
				buf+=formOptions(7,document.getElementById("sat"+i).checked);
				resJSON+='"options"'+":"+buf+"}";
				if (i<range-1)												//это последняя программа?
					resJSON+=", ";
			}
			resJSON+="}";
			console.log(resJSON);
			return resJSON;
		}
		//выводим информационное сообщение. На входе само сообщение и его цвет
		function printt_message(message, color){
			document.getElementById("info_msg").textContent= message;
			document.getElementById("info_msg").style.color=color;
		}
		var text = '{"program0": {"hour_on":1, "min_on":11,"hour_off":1, "min_off":11, "options":128},"program1":{"hour_on":2, "min_on":22,"hour_off":2, "min_off":22, "options":85},"program2":{"hour_on":3, "min_on":33,"hour_off":3, "min_off":33, "options":85},"program3":{"hour_on":4, "min_on":44,"hour_off":4, "min_off":44, "options":85},"program4":{"hour_on":5, "min_on":55,"hour_off":5, "min_off":5, "options":85}}';
		var sendBTN = document.getElementById("send_button");
		// формируем JSON и отрпавляем его на сервер------------------
		function send_json() {
			var request = new XMLHttpRequest(); 					//Создаём новый объект XMLHttpRequest
			request.open('POST','/set_programms',false);			//Конфигурируем его: POST-запрос на URL '/set_programms', false - запрос будет выполнен синхронно
			printt_message("нет соединения с сервером","red");
			request.send(formJSON());								//Отсылаем запрос, передаем строку в формате JSON на сервер
			if (request.readyState == 4 && request.status == 200) 	//если запрос завершен и завершен успешно
				printt_message("Изменения успешно внесены","green");
			else
				printt_message("Ошибка отправки данных на сервер","red");
		}
		var refreshBTN=document.getElementById("refresh_button");
		// обновляем  страницу, со значениями полученными с сервера---
		function refresh_page(){
			var request = new XMLHttpRequest();						//Создаём новый объект XMLHttpRequest
			request.open('GET','/get_programms',false);				//Конфигурируем его: GET-запрос на URL '/get_programms', false - запрос будет выполнен синхронно
			printt_message("нет соединения с сервером","red");
			request.send();											//Отсылаем запрос
			if (request.readyState == 4 && request.status == 200) {	//если запрос завершен и завершен успешно
				getProgramms(request.responseText);					//заполняем страницу полученными значениями
				printt_message("Данные успешно обновлены","green");
			}
			else
				printt_message("Ошибка получения данных с сервера","red");
		}
		
		sendBTN.addEventListener('click', send_json);
		refreshBTN.addEventListener('click', refresh_page);
		
		var synctimeBTN=document.getElementById("sync_time");
		//отправляем запрос синхронизации времени на сервер
		function sync_time(){
			var request = new XMLHttpRequest();						//Создаём новый объект XMLHttpRequest
			request.open('GET','/sync_time',false);					//Конфигурируем его: GET-запрос на URL '/sync_time', false - запрос будет выполнен синхронно
			printt_message("нет соединения с сервером","red");
			request.send();											//Отсылаем запрос
			if (request.readyState == 4 && request.status == 200) {	//если запрос завершен и завершен успешно
				printt_message("Время синхронизировано","green");
				document.getElementById("current_time").textContent=request.responseText;
			}
			else
				printt_message("Ошибка синхронизации времени","red");
		}
		synctimeBTN.addEventListener('click', sync_time);
		//включение или отключение в ручном режиме
		var switchBTN1=document.getElementById("switch_button1");
		function switch_relay(){
			var request = new XMLHttpRequest();						//Создаём новый объект XMLHttpRequest
			request.open('GET','/switch_relay',false);				//Конфигурируем его: GET-запрос на URL '/switch_relay', false - запрос будет выполнен синхронно
			printt_message("нет соединения с сервером","red");
			request.send();											//Отсылаем запрос
			if (request.readyState == 4 && request.status == 200) {	//если запрос завершен и завершен успешно
				printt_message(". . .");
				if (request.responseText){
					document.getElementById("switch_button1").style.backgroundColor = "olive";
					document.getElementById("switch_button1").value="on";
				}
				else{
					document.getElementById("switch_button1").style.backgroundColor = "coral";
					document.getElementById("switch_button1").value="off";
				}
			}
			else
				printt_message("Запрос не обработан","red");
		}
		
		
		switchBTN1.addEventListener('click', switch_relay);
		//запрос текущего состояния реле
		function check_status_power(){
			var request = new XMLHttpRequest();						//Создаём новый объект XMLHttpRequest
			request.open('GET','/status_power',false);				//Конфигурируем его: GET-запрос на URL '/status_power', false - запрос будет выполнен синхронно
			request.send();											//Отсылаем запрос
			if (request.readyState == 4 && request.status == 200) {	//если запрос завершен и завершен успешно
				if (request.responseText){
					document.getElementById("switch_button1").style.backgroundColor = "olive";
					document.getElementById("switch_button1").value="on";
				}
				else{
					document.getElementById("switch_button1").style.backgroundColor = "coral";
					document.getElementById("switch_button1").value="off";
				}
			}
		}
		//получение даты и времени с сервера
		function get_time(){
			var request = new XMLHttpRequest();						//Создаём новый объект XMLHttpRequest
			request.open('GET','/get_time',false);					//Конфигурируем его: GET-запрос на URL '/get_time', false - запрос будет выполнен синхронно
			request.send();											//Отсылаем запрос
			if (request.readyState == 4 && request.status == 200) {	//если запрос завершен и завершен успешно
				document.getElementById("current_time").textContent=request.responseText;
			}
		}
		//проверяем корректность введенных значений и блокируем кнопку, если данные не корректны
		function validForm(f){
			if (!f.validity.valid){
				document.getElementById("send_button").disabled = true;
				printt_message("введены некорректные данные","red");
			}
			else{
				document.getElementById("send_button").disabled = false;
				printt_message("","green");
			}
		}
		//запускается при загрузке страницы
		function check_status_all(){
			check_status_power();
			refresh_page();
			get_time();
		}
		
		//getProgramms(text);
		//formJSON();
		//getProgramms(formJSON());