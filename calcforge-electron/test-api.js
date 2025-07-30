// Simple test script to verify syntax highlighting API
const fetch = require('node-fetch');

async function testSyntaxHighlighting() {
    try {
        console.log('Testing syntax highlighting API...');
        
        const response = await fetch('http://localhost:8000/api/syntax-highlight', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ 
                text: 'sqrt(16) + LN1 * 2' 
            })
        });
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const data = await response.json();
        console.log('Success! Syntax highlighting response:');
        console.log(JSON.stringify(data, null, 2));
        
        // Test with comment
        const commentResponse = await fetch('http://localhost:8000/api/syntax-highlight', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ 
                text: ':::This is a comment\nsqrt(25) + 10' 
            })
        });
        
        const commentData = await commentResponse.json();
        console.log('\nComment test response:');
        console.log(JSON.stringify(commentData, null, 2));
        
    } catch (error) {
        console.error('API test failed:', error);
    }
}

testSyntaxHighlighting();
