package;

class Progress
{
   public var current:Int;
   public var total:Int;

   public function new(inCurrent:Int, inTotal:Int)
   {
      current = inCurrent;
      total = inTotal;
   }

   public function getProgress():String
   {
      var percent = current / total;
      var pct = Std.int(percent * 1000) / 10;
      var progress = Std.string(pct);

      if (Std.int(pct) == pct)
         progress += ".0";

      return progress + "%";
   }

   public function getFormattedProgress():String
   {
      var progress = getProgress();

      var spaces = "";

      while ((spaces.length + progress.length) < 6)
         spaces += " ";

      return "\x1b[1m[\x1b[0m" + spaces + "\x1b[33;1m" + progress + "\x1b[0m\x1b[1m]\x1b[0m";
   }
}
