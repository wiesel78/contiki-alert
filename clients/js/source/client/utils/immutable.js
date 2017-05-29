
// aktualisieren vorhandener und hinzufügen neuer einträge in ein bestehendes Array oder Objects
// arr : basis array
// inserts : neue bzw. zu aktualisierende elemente
// selector : selektiert innerhalb eines objektes ein element, anhand dessen die objekte in den listen unterschieden werden
//      standardmäßig, wird das object selbst ausgegeben
export const immutableSave = (target, inserts, selector) =>
{
    if( target instanceof Array )
        return immutableArraySave(target, inserts, selector);

    if( target instanceof Object )
        return immutableObjectSave(target, inserts, selector);

    return target;
};


// aktualisieren vorhandener und hinzufügen neuer einträge in ein bestehendes Array
// arr : basis array
// inserts : neue bzw. zu aktualisierende elemente
// selector : selektiert innerhalb eines objektes ein element, anhand dessen die objekte in den listen unterschieden werden
//      standardmäßig, wird das object selbst ausgegeben
export const immutableArraySave = (arr = [], inserts = [], selector = x => x) =>
{
    // Trivia 1
    if( inserts.length <= 0 )
        return [ ...arr ];

    // Trivia 2
    if( arr.length <= 0 )
        return [ ...inserts ];

    // neues array aus dem alten erzeugen
    var newArr = [ ...arr ];

    // alle neueinträge durchgehen und zwischen updates und inserts unterscheiden
    inserts.forEach(x => {
        var index = arr.findIndex(y => selector(y) === selector(x));

        // wenn das element im alten Array nicht existierte wird ein einfacher das neue einfach hinzugefügt
        // falls es bereits existierte, wird das alte element ersetzt
        if(index === -1)
        {
            newArr.push(x);
        }
        else
        {
            newArr[index] = x;
        }
    });

    return newArr;
};

// aktualisieren vorhandener und hinzufügen neuer einträge in ein bestehendes Object
// arr : basis object
// inserts : neue bzw. zu aktualisierende elemente
// selector : selektiert innerhalb eines objektes ein element, anhand dessen die objekte in den listen unterschieden werden
//      standardmäßig, wird das object selbst ausgegeben
export const immutableObjectSave = (target = {}, inserts = {}, selector = x => x) =>
{
    // Trivia 1
    if( inserts.length <= 0 )
        return Object.assign({}, target);

    // Trivia 2
    if( target.length <= 0 )
        return Object.assign({}, inserts);

    // neues array aus dem alten erzeugen
    var newTarget = Object.assign({}, target);

    // alle neueinträge durchgehen und zwischen updates und inserts unterscheiden
    inserts.forEach(x => newTarget[selector(x)] = x);

    return newTarget;
};
