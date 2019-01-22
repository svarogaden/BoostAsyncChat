window.onload = function ()
{



 var wsUri = "ws://127.0.0.1:1234";
 var websocket;


 var label = document.getElementById("status");
 var message = document.getElementById("message");
 var btnSend = document.getElementById("send");
 var btnStop = document.getElementById("stop");
 var btnConnect = document.getElementById("connect");

 var msgs = document.getElementById("msgs");
 var name = document.getElementById("name");


 var reconnect=0;


   function User (name,message)
    {
        this.name = name;
        this.message = message ;
    }





function ConnectWebSocket()
{
        websocket = new WebSocket(wsUri);
        websocket.addEventListener('open', socketOpenListener);
        websocket.addEventListener('message', socketMessageListener);
        websocket.addEventListener('close', socketCloseListener);
        websocket.addEventListener('error', socketErrorListener);    
 }



const socketOpenListener = (event) => 
{
   label.innerHTML = 'Connected! ' + websocket.url; 
   console.log('Connected');
};

const socketMessageListener = (event) => 
{



        if (typeof event.data === 'string')
        {
            
            console.log(event.data);


            var strMsg = '';
            var user = null;
            var name = m = '';

         
            try
            {
                 user = JSON.parse(event.data);
                 name = user.name;
                 m = user.message;

                  strMsg = '<p><strong>' + name + '</strong> : ' + m  + '-' + Date() + '</p>';
                 

            }
            catch(e)
            {
                strMsg = '<p><em>' + event.data +  ' - ' + Date() +  '</em></p>';

            }
            msgs.innerHTML = strMsg;
           
        } 
};

const socketCloseListener = (event) =>
 {
     
      var wasClean = event.wasClean;


        if (wasClean)
        {
            label.innerHTML = 'Connection closed correctly :  ' + event.code;
        }
         else
        {
           
            label.innerHTML = 'Connection closed with an error  : ' + event.code; 
            ConnectWebSocket();  
            reconnect++;
            msgs.innerHTML += "<p>Reconect! - " + reconnect + " - " + Date() + "<p>";
         }

  };

const  socketErrorListener =  (event)=>
{
    label.innerHTML = 'ERROR!  ' + event.code;
};







  ConnectWebSocket();







  setInterval(()=>{



  
 if (websocket.readyState === WebSocket.OPEN)
{

   websocket.send("Stres test - " + Date());
}


},50);







    
  
   
    btnSend.onclick = function ()
    {
        if (websocket.readyState === WebSocket.OPEN)
        {
           
         var user = new User(name.value, message.value); 


        

       
         websocket.send(JSON.stringify(user));
      

            
        }
    
    };


    btnStop.onclick = function () {
        if (websocket.readyState === WebSocket.OPEN)
            websocket.close();

    };



       btnConnect.onclick = function () {
     

        if (websocket.readyState === WebSocket.CLOSED)
        {
           ConnectWebSocket();        
        }

    };


};