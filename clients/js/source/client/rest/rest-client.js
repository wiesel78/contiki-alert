
const JsonHeader = {
    "Content-type" : "application/json"
};

const JsonOption = {
    headers : JsonHeader
};

const getOption = data => Object.assign(
    {
        method : "get" ,
        credentials: "include"
    }, JsonOption);
const postOption = data => Object.assign(
    {
        method : "post",
        body : JSON.stringify(data || {}) ,
        credentials : "include"
    }, JsonOption);

// filtert fehlerhafte antworten in einer thenable kette
function status(response) {
    if(response.status >= 200 && response.status < 300)
        return Promise.resolve(response);
    else
        return Promise.reject(new Error(response.statusText));
}

// macht aus einem reponseobjekt ein json objekt sobald alle daten da sind
function json(response) {
    return response.json();
}

// Client hilsfunktion für get anfragen
export const get = (url) => {
    var promise = new Promise((resolve, reject) => {
        fetch(url, getOption())
            .then(status)
            .then(json)
            .then(resolve)
            .catch(response =>
                reject(new Error(response.statusText))
            );
        });

    return promise;
}

// Client hilfsfunktion für post anfragen
export const post = (url, data) => {
    var promise = new Promise((resolve, reject) => {

        fetch(url, postOption(data))
            .then(status)
            .then(json)
            .then(resolve)
            .catch(response =>
                reject(new Error(response.statusText))
            );
    });

    return promise;
}
