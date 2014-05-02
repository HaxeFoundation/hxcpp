import massive.munit.client.RichPrintClient;
import massive.munit.client.HTTPClient;
import massive.munit.client.SummaryReportClient;
import massive.munit.TestRunner;
class TestMain {

    static function main(){ new TestMain(); }

    public function new() {
        var suites = new Array<Class<massive.munit.TestSuite>>();
        suites.push(TestSuite);

        var client = new RichPrintClient();
        var httpClient = new HTTPClient(new SummaryReportClient());

        var runner:TestRunner = new TestRunner(client); 
        runner.addResultClient(httpClient);

        runner.completionHandler = completionHandler;
        runner.run(suites);
        Sys.exit(0);
    }

    function completionHandler(successful:Bool):Void {}
}
