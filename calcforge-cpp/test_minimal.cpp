#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QWidget window;
    window.setWindowTitle("Test Qt App");
    
    QVBoxLayout *layout = new QVBoxLayout(&window);
    QLabel *label = new QLabel("Hello Qt!", &window);
    layout->addWidget(label);
    
    window.show();
    
    return app.exec();
}
