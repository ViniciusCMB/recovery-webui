
async function get_port_options() {
    // (...)
    const response = await fetch("/api/config/get_port_options")
    const result = await response.json()
    return result["ports_avaliable"]
}

async function get_baudrate_options() {
    // (...)
    const response = await fetch("/api/config/get_baudrate_options")
    const result = await response.json()
    return result["baudrates_avaliable"]
}

async function get_timeout_options() {
    // (...)
    const response = await fetch("/api/config/get_timeout_options")
    const result = await response.json()
    return result["timeouts_avaliable"]
}

// (to hermes: I'm going to finish the this later, leave me alone...)

// ------ Old!
// async function get_serial_port_list() {
//     console.log("fetch_serial_list ativado");
//     const response = await fetch("/api/get_serial_ports");
//     if ( response.ok ) {
//         const result = await response.json();
//         console.log(result);
//         return result["ports_avaliable"];
//     } else {
//         console.error("fetching serial list error");
//     }
// }
//
// async function set_serial_list() {
//     console.log("set_serial ativado!")
//     serial_list = await get_serial_port_list();
//     serial_list.forEach( serial => console.log(serial) );
//
//     let serial_list_element = document.querySelector("#serial_port_field");
//
//     serial_list.forEach( ( serial ) => {
//         let element = document.createElement("option");
//         element.value = serial;
//         element.textContent = serial;
//         serial_list_element.append(element);
//     });
// }
//
// function clear_serial_list() {
//     console.log("clear_serial_list ativado");
//
//     let serial_list_element = document.querySelector("#serial_port_field");
//
//     console.log(serial_list_element.childNodes)
// }
//
// function refresh_serial_list() {
//     clear_serial_list();
//     set_serial_list();
// }
//
// function disconnect_serial() {
//     console.log("disconnect_serial ativado");
//     // ...
// }
//
// function send_new_config() {
//     let new_serial_port = document.querySelector("#serial_port_field").value;
//     let new_baudrate = document.querySelector("#baudrate_field").value;
//     let new_timeout = document.querySelector("#timout_field").value;
//     console.log(new_serial_port);
//     console.log(new_baudrate);
//     console.log(new_timeout);
// }
//
// set_serial_list();
//
