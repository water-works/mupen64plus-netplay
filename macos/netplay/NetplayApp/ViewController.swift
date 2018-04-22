import Cocoa
import Netplay

class ViewController: NSViewController {

    override func viewDidLoad() {
        super.viewDidLoad()

        // Print out "Hello, world!" and test Swift deps!
        NSLog(NetplayClient().greet())

        // Do any additional setup after loading the view.
    }

    override var representedObject: Any? {
        didSet {
            // Update the view, if already loaded.
        }
    }

}
