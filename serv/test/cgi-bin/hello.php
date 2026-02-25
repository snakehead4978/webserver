<?php
$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
$query  = $_SERVER['QUERY_STRING'] ?? '';
$name   = $_GET['name'] ?? 'world';

$body  = "<html><body>";
$body .= "<h1>Hello from PHP, " . htmlspecialchars($name) . "!</h1>";
$body .= "<p>METHOD: " . $method . "</p>";
$body .= "<p>QUERY_STRING: " . $query . "</p>";
$body .= "<p>SERVER_PORT: " . ($_SERVER['SERVER_PORT'] ?? '') . "</p>";

if ($method === 'POST') {
    $post_data = file_get_contents('php://stdin');
    $body .= "<p>POST body: " . htmlspecialchars($post_data) . "</p>";
}

$body .= "</body></html>";

header("Content-Type: text/html");
header("Content-Length: " . strlen($body));
echo $body;
?>
