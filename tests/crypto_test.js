/* Crypto test script */
console.log("Testing Crypto API...");

if (typeof crypto === 'undefined') {
    console.log("FAIL: crypto object not found");
} else {
    console.log("PASS: crypto object found");

    // Test getRandomValues
    try {
        var array = new Uint32Array(10);
        crypto.getRandomValues(array);
        console.log("getRandomValues output: " + array);
        var sum = 0;
        for (var i = 0; i < array.length; i++) sum += array[i];
        if (sum > 0) {
            console.log("PASS: getRandomValues produced non-zero output");
        } else {
            console.log("FAIL: getRandomValues produced all zeros");
        }
    } catch (e) {
        console.log("FAIL: getRandomValues threw error: " + e);
    }

    // Test subtle.digest
    if (typeof crypto.subtle === 'undefined') {
        console.log("FAIL: crypto.subtle not found");
    } else {
        console.log("PASS: crypto.subtle found");

        var data = new Uint8Array([1, 2, 3, 4, 5]);
        crypto.subtle.digest("SHA-256", data).then(function(hash) {
            var hex = Array.from(new Uint8Array(hash)).map(b => b.toString(16).padStart(2, '0')).join('');
            console.log("SHA-256 Digest: " + hex);
            // SHA-256 of [1,2,3,4,5] is 74f395450989a269153c13b2ad879b3792c823f3957ed6c62f222041935639f7
            if (hex === "74f395450989a269153c13b2ad879b3792c823f3957ed6c62f222041935639f7") {
                console.log("PASS: SHA-256 digest is correct");
            } else {
                console.log("FAIL: SHA-256 digest is incorrect");
            }
        }).catch(function(err) {
            console.log("FAIL: subtle.digest threw error: " + err);
        });
    }
}
