using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Media;

namespace Transition_Effect.Converters
{
   public class EffectBackground:IValueConverter
    {
       static Random rnd = new Random();
        public object Convert(object value, Type targetType, object parameter, string language)
        {
             byte temp = (byte)rnd.Next(100,200);
             return new SolidColorBrush(Color.FromArgb(255, temp, temp, temp));
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
